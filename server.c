#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <fcntl.h>
#include <wait.h>
#include <time.h>

#include <sqlite3.h>

#include <crypt.h>

#include "functions.h"
#include "database_functions.h"
#include "errors.h"
#include "server.h"

char *zErrMsg;
int dbproject;

char cmd_received[MAX_COMMAND]; // stores the command received from the client
char cmd_answer[MAX_ANSWER];    // stores the answer

int client;
SSL *ssl;

char *path;

int shell_exit() // returns -1 to terminate execution
{
    printf("Client disconnected\n");
    return -1;
}
int shell_logout()
{
    int check = get_logged_status();
    if (!check)
        strcat(cmd_answer, "Not logged in");
    if (check)
    {
        int ok;
        ok = update_logged_status();
        if (ok)
            strcat(cmd_answer, "User logged out");
        return 1;
    }
    return 0;
}

void current_directory()
{
    char directory[2000];
    getcwd(directory, sizeof(directory));
    strcat(cmd_answer, "Current directory: ");
    strcat(cmd_answer, directory);
}

int shell_cd(char **args) // implementation of cd
{
    // login - cd - other command ?????
    int check = get_logged_status(); // here
    if (check)
    {
        if (args[1] == NULL)
        {
            strcat(cmd_answer, "error - missing argument (cd)\n"); // printf -- client
            handle_error("missing argument - cd \n");
        }
        else
        {

            if (chdir(args[1]) != 0)
            {
                strcat(cmd_answer, "error - the directory doesn't exist (cd)\n"); // printf -- client
                handle_error("cd \n");
            }
            current_directory();
        }
        return 1;
    }
    else
    {
        SSL_write(ssl, "Not logged in - please login first", 35);
        return 0;
    }
}

int shell_login()
{
    int check = get_logged_status();
    if (check)
    {
        strcat(cmd_answer, "Someone is already logged in - please logout first!\n");
        return 0;
    }
    else
    {
        char username[30], password[40];
        bzero(username, 30);
        bzero(password, 40);
        sqlite3 *db;
        SSL_write(ssl, "Username: ", 11);
        SSL_read(ssl, username, 30);
        SSL_write(ssl, "Password: ", 11);
        SSL_read(ssl, password, 40);
        sqlite3_stmt *stmt;
        int rc;
        rc = sqlite3_open(path, &db);

        rc = sqlite3_prepare_v2(db, "SELECT password,salt FROM users WHERE username = ?", -1, &stmt, 0);
        if (rc)
        {
            fprintf(stderr, "Can't prepare SELECT statement: %s %d\n", sqlite3_errmsg(db), sqlite3_extended_errcode(db));
            return 0;
        }

        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (rc)
        {
            fprintf(stderr, "Can't bind username: %s\n", sqlite3_errmsg(db));
            return 0;
        }

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW)
        {
            char *stored_hash = (char *)sqlite3_column_text(stmt, 0);
            char *salt = (char *)sqlite3_column_text(stmt, 1);
            char *inputted_hash = crypt(password, salt);
            if (strcmp(inputted_hash, stored_hash) == 0)
            {

                sqlite3_finalize(stmt);
                sqlite3_close(db);
                update_logged_status(); // the user is logged in
                return 1;
            }
        }
        else if (rc == SQLITE_DONE)
        {
            SSL_write(ssl, "The username does not exist. Create a new account? [y/n]", 57);
            char ans[3];
            SSL_read(ssl, ans, 3);
            if (strcmp(ans, "y\n") == 0)
            {
                printf("Username %s", username);
                if (create_account(username, password))
                {
                    sqlite3_finalize(stmt);
                    sqlite3_close(db);
                    update_logged_status(); // the user is logged in
                    printf("Account created successfully.\n");
                    return 1;
                }
                else
                {
                    fprintf(stderr, "Error creating account.\n");
                    return 0;
                }
            }
            else
            {
                fprintf(stderr, "Error creating account.\n");
                return 0;
            }
        }
        else
        {
            fprintf(stderr, "execution of the SELECT statement");
            return 0;
        }

        bzero(username, 30);
        bzero(password, 40);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
}

int shell_launch(char *command) // executes system commands
{
    int check = get_logged_status();
    if (check)
    {
        int status;
        int pipe_output[2];
        if ((pipe(pipe_output)) == -1)
            handle_error_exit("[server] - pipe");

        pid_t pid = fork();
        if (pid == -1)
            handle_error_exit("[server] - fork");

        if (pid == 0)
        {
            // child
            // redirects output to pipe and gives the command answer to the parent process
            close(pipe_output[READ]);
            dup2(pipe_output[WRITE], STDOUT_FILENO);

            if (execl("/bin/bash", "/bin/bash", "-c", command, (char *)NULL) == -1)
                handle_error_exit("[server] - exec error");
            exit(10);
        }
        else
        {
            // parent
            close(pipe_output[WRITE]);
            waitpid(pid, &status, 0);
            int n = read(pipe_output[READ], cmd_answer, sizeof(cmd_answer)); // reads answer from pipe
            cmd_answer[n] = '\0';
        }
        return 1;
    }
    else
    {
        strcat(cmd_answer, "Not logged in - please login first");
        return 0;
    }
}

int shell_execution(char **args) // executes built in commands or launches system commands
{
    if (args[0] == NULL) // empty command
        return 1;
    int nr_commands = 4;
    char *command_list[nr_commands];
    int val;

    command_list[0] = "cd";
    command_list[1] = "exit";
    command_list[2] = "login";
    command_list[3] = "logout";
    for (int i = 0; i < nr_commands; i++)
    {
        if (strcmp(args[0], command_list[i]) == 0) // searching the command in the list
        {
            val = i + 1;
            break;
        }
    }
    switch (val)
    {
    case 1:
        return shell_cd(args);
    case 2:
        return shell_exit();
    case 3:
        if (shell_login())
        {
            strcat(cmd_answer, "Login successful");
            return 1;
        }
        else
        {
            strcat(cmd_answer, "Login unsuccessful");
            return 0;
        }
    case 4:
        return shell_logout();
    default:
        break;
    }
    return shell_launch(cmd_received);
}

char **shell_split_line(char *line)
{
    int poz = 0;
    char **args = malloc(BUFF_SIZE * sizeof(char *));
    char *p;
    if (!args)
        handle_error_exit("[server] - allocation in split_line");
    p = strtok(line, DELIM);
    while (p != NULL)
    {
        args[poz] = p;
        poz++;
        p = strtok(NULL, DELIM);
    }
    args[poz] = NULL;
    return args;
}

void shell_loop()
{
    char **args;
    int status;
    if (strstr(cmd_received, "cd") != NULL || strstr(cmd_received, "exit") != NULL || strstr(cmd_received, "login") != NULL || strstr(cmd_received, "logout") != NULL)
    {
        args = shell_split_line(cmd_received);
        status = shell_execution(args);
    }
    else
        status = shell_launch(cmd_received);
    SSL_write(ssl, cmd_answer, sizeof(cmd_answer));
    bzero(cmd_answer, sizeof(cmd_answer));
    bzero(cmd_received, sizeof(cmd_received));

    if (status >= 0) // the exit command returns a status<0, and then the execution stops
    {
        int val;
        val = SSL_read(ssl, cmd_received, MAX_COMMAND);
        cmd_received[val] = '\0';
        shell_loop(cmd_received);
    }
}

int open_listener(int port)
{
    struct sockaddr_in server;
    int sd; // socket descriptor

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        handle_error_exit("[server] - socket");

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
        handle_error_exit("[server] - bind");
    if (listen(sd, 5) == -1)
        handle_error_exit("[server] - listen");
    return sd;
}

int main()
{
    SSL_CTX *ctx;
    int server;
    sqlite3 *db;
    dbproject = sqlite3_open("projectdb.db", &db);
    if (dbproject != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    char *sql1; // users table creation
    sql1 = "CREATE TABLE USERS("
           "USERNAME TEXT PRIMARY KEY NOT NULL, "
           "PASSWORD TEXT             NOT NULL, "
           "SALT     TEXT             NOT NULL);";

    char *sql2; // used to keep track of clients and in which one there is someone logged in
    sql2 = "CREATE TABLE LOGIN("
           "PID           INT PRIMARY KEY NOT NULL, "
           "LOGGED_STATUS INT             NOT NULL);";
    dbproject = sqlite3_exec(db, sql1, NULL, 0, &zErrMsg);
    dbproject = sqlite3_exec(db, sql2, NULL, 0, &zErrMsg);

    path=get_path();

    SSL_library_init();
    ctx = InitServerCTX();                             // initialize SSL
    LoadCertificates(ctx, "mycert.pem", "mycert.pem"); // load certs - by default file names are both "mycert.pem"
    server = open_listener(PORT);
    struct sockaddr_in from;
    bzero(&from, sizeof(from));

    int run = 1;

    while (run)
    {

        socklen_t length = sizeof(from);
        client = accept(server, (struct sockaddr *)&from, &length);
        if (client < 0)
            handle_error_exit("[server] - accept");
        pid_t pid;
        ssl = SSL_new(ctx); // get new SSL state with context
        SSL_set_fd(ssl, client);
        if ((pid = fork()) == -1)
            handle_error_exit("[server] - fork");
        if (pid == 0) // child process
        {
            if (SSL_accept(ssl) == -1) // do SSL-protocol accept
                ERR_print_errors_fp(stderr);
            else
            {
                printf("New client connected \n"); // insert client pid in database
                int pid = getpid();
                int check = insert_client(pid);
                if (check)
                    while (1)
                    {
                        SSL_read(ssl, cmd_received, MAX_COMMAND);
                        cmd_received[sizeof(cmd_received)] = '\0';
                        shell_loop();
                    }
                else
                    SSL_write(ssl, "Some error occured", 19);
            }
            int sd;
            sd = SSL_get_fd(ssl); // get socket connection
            SSL_free(ssl);        // release SSL state
            close(sd);            // close connection
        }
    }
    close(client);
}