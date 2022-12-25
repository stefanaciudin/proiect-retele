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

int size_cd;
int pipe_number = -1; // stores the number of pipes
int fd_input, fd_output;
int has_input = 0, has_output = 0, append = 0, error_append = 0; // checks if the command has redirection

int shell_exit(char **args) // returns 0 to terminate execution
{
    printf("Client disconnected\n");
    return -1;
}

int shell_help(char **args)
{
    strcpy(cmd_answer, "For help info on commands, use the man command \n"); // printf -- client
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

int shell_launch(char **args) // executes system commands
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
        if (has_output) // redirects output to given file
        {

            if (!error_append) // ">" or ">>"
            {
                dup2(fd_output, STDOUT_FILENO);
                close(fd_output);
            }
            else
            {
                dup2(fd_output, STDERR_FILENO); // "2>" or "2>>"
                close(fd_output);
            }
        }

        if (!has_output) // redirects output to pipe and gives the command answer to the parent process
        {
            close(pipe_output[READ]);
            dup2(pipe_output[WRITE], STDOUT_FILENO);
        }
        if (execvp(args[0], args) == -1)
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
        return shell_help(args);
    case 2:
        return shell_cd(args);
    case 3:
        return shell_exit(args);
    default:
        break;
    }
    return shell_launch(args);
}

char **shell_split_line(char *line)
{
    int bufsize = BUFF_SIZE;
    int poz = 0;
    char **args = malloc(bufsize * sizeof(char *));
    char *p;
    char **args_copy;

    if (!args)
        handle_error("allocation in split_line");
    p = strtok(line, DELIM);
    while (p != NULL)
    {
        args[poz] = p;
        poz++;

        if (poz >= bufsize)
        {
            bufsize += BUFF_SIZE;
            args_copy = args;
            args = realloc(args, bufsize * sizeof(char *));
            if (!args)
            {
                free(args_copy);
                handle_error("allocation in split_line");
            }
        }

        p = strtok(NULL, DELIM);
    }
    args[poz] = NULL;
    return args;
}
char *shell_redirect(char *line)
{
    char check[MAX_COMMAND];
    char input[128], output[128];

    for (int i = 0; i < strlen(line); i++)
    {
        strcpy(check, &line[i]);
        if (check[0] == '<')
        {
            line[i] = '\0';
            strcpy(input, &line[i + 1]); // input file
            has_input++;
        }
        if (check[0] == '>')
        {
            if (check[1] == '>') //">>"
            {
                append++;
                line[i] = '\0';
                line[i + 1] = '\0';
                strcpy(output, &line[i + 2]);
                has_output++;
            }
            else //">"
            {
                line[i] = '\0';
                strcpy(output, &line[i + 1]); // output file
                has_output++;
            }
        }
        if (check[0] == '2')
        {
            if (check[1] == '>' && check[2] != '>') //"2>"
            {
                error_append++;
                line[i] = '\0';
                line[i + 1] = '\0';
                strcpy(output, &line[i + 2]);
                has_output++;
            }
            else if (check[1] == '>' && check[2] == '>') //"2>>"
            {
                error_append++;
                append++;
                line[i] = '\0';
                line[i + 1] = '\0';
                line[i + 2] = '\0';
                strcpy(output, &line[i + 3]);
                has_output++;
            }
        }
    }

    if (has_input)
    {
        if ((fd_input = open(input, O_RDONLY)) < 0)
            handle_error_ret("opening the input file");
        dup2(fd_input, STDIN_FILENO);
        close(fd_input);
    }
    if (has_output)
    {
        if (error_append && !append) //"2>"
        {
            if ((fd_output = creat(output, 0644)) < 0)
                handle_error("creating the output file");
        }
        else if ((error_append && append) || append) //"2>>" or ">>"
        {
            if ((fd_output = open(output, O_RDWR | O_APPEND | O_CREAT)) < 0) // if the file exists, we append to it
                handle_error("creating the output file");
        }
        else if ((fd_output = creat(output, 0644)) < 0) // read write permissions for user and group, read permission for everyone else; ">"
            handle_error("creating the output file");
    }
    return line;
}

void shell_loop(char *command)
{
    char **args;
    int status;
    command = shell_redirect(command);
    args = shell_split_line(command);
    status = shell_execution(args);

    SSL_write(ssl, cmd_answer, sizeof(cmd_answer));
    bzero(cmd_answer, sizeof(cmd_answer));
    bzero(cmd_received, sizeof(cmd_received));
    bzero(command, MAX_COMMAND);
    has_output = 0, has_input = 0, append = 0, error_append = 0;
    pipe_number = -1;
    free(args);

    if (status >= 0) // the exit command returns a status<0, and then the execution stops
    {
        bzero(cmd_received, sizeof(cmd_received));
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
                    shell_loop(cmd_received);
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