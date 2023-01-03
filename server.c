#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <wait.h>

#include <sqlite3.h>

#include "functions.h"
#include "errors.h"

sqlite3 *db;
char *zErrMsg;
int dbproject;

char cmd_received[MAX_COMMAND]; // stores the command received from the client
char cmd_answer[MAX_ANSWER];    // stores the answer

int client;
SSL *ssl;

int shell_exit() // returns -1 to terminate execution
{
    printf("Client disconnected\n");
    return -1;
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
    if (args[1] == NULL)
    {
        strcat(cmd_answer, "error - missing argument (cd)\n"); // printf -- client
        handle_error_ret("missing argument - cd \n");
    }
    else
    {

        if (chdir(args[1]) != 0)
        {
            strcat(cmd_answer, "error - cd\n"); // printf -- client
            handle_error_ret("cd \n");
        }
        current_directory();
    }
    return 1;
}
int create_account(char *username, char *password)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("projectdb.db", &db);
    if (rc)
        handle_error_ret("can't open database");

    rc = sqlite3_prepare_v2(db, "INSERT INTO users (username, password) VALUES (?, ?)", -1, &stmt, 0);
    if (rc)
        handle_error_ret("can't prepare INSERT statement");

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    if (rc)
        handle_error_ret("can't bind username");

    rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    if (rc)
        handle_error_ret("can't bind password");

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        handle_error_ret("error inserting row");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

int shell_login()
{
    char username[30], password[40];
    SSL_write(ssl, "Username: ", 11);
    SSL_read(ssl, username, 30);
    SSL_write(ssl, "Password: ", 11);
    SSL_read(ssl, password, 40);
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(db, "SELECT password FROM users WHERE username = ?", -1, &stmt, 0);
    if (rc)
        handle_error_ret("can't prepare SELECT statement");

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    if (rc)
        handle_error_ret("can't bind username");

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        char *stored_password = (char *)sqlite3_column_text(stmt, 0);
        if (strcmp(password, stored_password) == 0)
        {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }
    }
    else if (rc == SQLITE_DONE)
    {
        SSL_write(ssl, "The username does not exist. Create a new account? [y/n]", 57);
        char ans[3];
        SSL_read(ssl, ans, 3);
        ans[sizeof(ans)] = '\0';
        printf("%s",ans);
        if (strcmp(ans, "y\n") == 0)
        {
            if (create_account(username, password))
            {
                printf("Account created successfully.\n");
                return 1;
            }
            else
                handle_error_ret("creating the new account");
        }
    }
    else
        handle_error_ret("execution of the SELECT statement");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int shell_launch(char *command) // executes system commands
{
    int status;
    int pipe_output[2];
    if ((pipe(pipe_output)) == -1)
        handle_error_exit("pipe");

    pid_t pid = fork();
    if (pid == -1)
        handle_error_exit("fork");

    if (pid == 0)
    {
        // child
        // redirects output to pipe and gives the command answer to the parent process
        close(pipe_output[READ]);
        dup2(pipe_output[WRITE], STDOUT_FILENO);

        if (execl("/bin/bash", "/bin/bash", "-c", command, (char *)NULL) == -1)
            handle_error_exit("exec error");
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

int shell_execution(char **args) // executes built in commands or launches system commands
{
    if (args[0] == NULL) // empty command
        return 1;
    int nr_commands = 3;
    char *command_list[nr_commands];
    int val;

    command_list[0] = "cd";
    command_list[1] = "exit";
    command_list[2] = "login";
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
        handle_error_exit("allocation in split_line");
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
    if (strstr(cmd_received, "cd") != NULL)
    {
        args = shell_split_line(cmd_received);
        status = shell_execution(args);
    }
    else if (strstr(cmd_received, "exit") != NULL)
    {
        args = shell_split_line(cmd_received);
        status = shell_execution(args);
    }
    else if (strstr(cmd_received, "login") != NULL)
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
        handle_error_exit("socket");

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
        handle_error_exit("bind");
    if (listen(sd, 5) == -1)
        handle_error_exit("listen");
    return sd;
}

int main()
{
    SSL_CTX *ctx;
    int server;

    dbproject = sqlite3_open("projectdb.db", &db);
    if (dbproject != SQLITE_OK)
        handle_error_ret("database");

    char *sql1; // table creation
    sql1 = "CREATE TABLE USERS("
           "USERNAME TEXT PRIMARY KEY NOT NULL, "
           "PASSWORD TEXT             NOT NULL);";
    // execute
    dbproject = sqlite3_exec(db, sql1, NULL, 0, &zErrMsg);

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
            handle_error_exit("accept");
        pid_t pid;
        ssl = SSL_new(ctx);      // get new SSL state with context
        SSL_set_fd(ssl, client);
        if ((pid = fork()) == -1)
            handle_error_exit("fork");
        if (pid == 0) // child process
        {
            if (SSL_accept(ssl) == -1) // do SSL-protocol accept
                ERR_print_errors_fp(stderr);
            else
            {
                printf("New client connected \n");
                while (1)
                {
                    SSL_read(ssl, cmd_received, MAX_COMMAND);
                    cmd_received[sizeof(cmd_received)] = '\0';
                    shell_loop();
                }
            }
            int sd;
            sd = SSL_get_fd(ssl); // get socket connection
            SSL_free(ssl);        // release SSL state
            close(sd);            // close connection
        }
        // else
        // {
        //     close(child_to_parent[WRITE]);
        //     char info[3000];
        //     int n = read(child_to_parent[READ], info, 1000);
        //     info[n] = '\0';
        //     write(client, info, strlen(info));
        // }
    }
    close(client);
}