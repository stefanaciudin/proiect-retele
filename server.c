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

#include "functions.h"

char cmd_received[MAX_COMMAND]; // stores the command received from the client
char cmd_answer[MAX_ANSWER];    // stores the answer

int client;
SSL *ssl;

int shell_exit() // returns -1 to terminate execution
{
    printf("Client disconnected\n");
    return -1;
}

int shell_help()
{
    strcat(cmd_answer, "For help info on commands, use the man command \n"); // printf -- client
    return 1;
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

int shell_launch(char *command) // executes system commands
{
    int status;
    int pipe_output[2];
    if ((pipe(pipe_output)) == -1)
        handle_error("pipe");

    pid_t pid = fork();
    if (pid == -1)
        handle_error("fork");

    if (pid == 0)
    {
        // child
        // redirects output to pipe and gives the command answer to the parent process
        close(pipe_output[READ]);
        dup2(pipe_output[WRITE], STDOUT_FILENO);

        if (execl("/bin/bash", "/bin/bash", "-c", command, (char *)NULL) == -1)
            handle_error("exec error");
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

    command_list[0] = "help";
    command_list[1] = "cd";
    command_list[2] = "exit";
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
        return shell_help();
    case 2:
        return shell_cd(args);
    case 3:
        return shell_exit();
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
        handle_error("allocation in split_line");
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
    if (strstr(cmd_received, "cd") != NULL) //also login :)
        {
            args = shell_split_line(cmd_received);
            status=shell_execution(args);
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
        handle_error("socket");

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
        handle_error("bind");
    if (listen(sd, 5) == -1)
        handle_error("listen");
    return sd;
}

int main()
{
    SSL_CTX *ctx;
    int server;

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
            handle_error("accept");
        pid_t pid;
        ssl = SSL_new(ctx);      // get new SSL state with context
        SSL_set_fd(ssl, client); // might have to move this after fork, pls check
        if ((pid = fork()) == -1)
            handle_error("fork");
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