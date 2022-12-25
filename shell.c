#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_COMMAND 1024 // max number of letters in a command
#define MAXLIST 100      // max number of commands

#define READ 0
#define WRITE 1

int pipe_number = -1;
int fd_input, fd_output;
int has_input = 0, has_output = 0;

void current_directory()
{
    char directory[1024];
    getcwd(directory, sizeof(directory));
    printf("%s ", directory);
}
int shell_exit(char **args) // returns 0 to terminate execution
{
    printf("Closing shell \n");
    return 0;
}

int shell_cd(char **args) // implementation of cd
{
    if (args[1] == NULL)
        perror("missing argument - cd \n");
    else
    {
        if (chdir(args[1]) != 0)
            perror("error - cd\n");
    }
    return 1;
}

int shell_help(char **args) // help mom
{
    printf("For help info on commands, use the man command \n");
}

int shell_launch(char **args) // executes system commands
{
    pid_t pid = fork();
    int status;
    if (pid == -1)
        perror("error - fork \n");
    if (pid == 0)
    {
        // child
        if (has_output)
        {
            dup2(fd_output, STDOUT_FILENO);
            close(fd_output);
        }
        if (execvp(args[0], args) == -1)
            perror("error - execvp \n");
        exit(10);
    }
    else
    {
        // parent
        waitpid(pid, &status, 0);
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

char *shell_read_line()
{
    char *buffer;
    buffer = readline(">>");
    if (strlen(buffer))
        add_history(buffer);
    return buffer;
}

#define BUFF_SIZE 256
#define DELIM " \n"

char **shell_split_line(char *line)
{
    int bufsize = BUFF_SIZE;
    int poz = 0;
    char **args = malloc(bufsize * sizeof(char *));
    char *p;
    char **args_copy;

    if (!args)
    {
        fprintf(stderr, "split_line - allocation error\n");
        exit(1);
    }

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
                fprintf(stderr, "split_line - allocation error\n");
                exit(1);
            }
        }

        p = strtok(NULL, DELIM);
    }
    args[poz] = NULL;
    return args;
}

char *shell_redirect(char *line)
{
    char test[5000];
    char input[128], output[128];

    // finds where redirection occurs
    for (int i = 0; i < strlen(line); i++)
    {
        strcpy(test, &line[i]);
        // printf("%d\n ",test[0]);
        if (test[0] == '<')
        {
            line[i] = '\0';
            strcpy(input, &line[i + 1]); // input file
            has_input++;
        }
        if (test[0] == '>')
        {
            line[i] = '\0';
            strcpy(output, &line[i + 1]); // output file
            has_output++;
        }
    }

    if (has_input)
    {
        if ((fd_input = open(input, O_RDONLY, 0)) < 0)
        {
            perror("error - opening the input file");
            exit(1);
        }
        dup2(fd_input, STDIN_FILENO);
        close(fd_input);
    }
    if (has_output)
    {
        if ((fd_output = creat(output, 0644)) < 0) // read write permissions
        {
            perror("error - opening the output file");
            exit(1);
        }
    }

    return line;
}

char **shell_split_pipes(char *line)
{
    int bufsize = BUFF_SIZE;
    int poz = 0;
    char **to_split = malloc(bufsize * sizeof(char *));
    char *p;
    p = strtok(line, "|");
    while (p != NULL)
    {
        to_split[poz] = p;
        to_split[poz][strlen(p)] = '\0';
        printf("%s%ld ", to_split[poz], strlen(p));
        poz++;
        pipe_number++;
        // to_split[poz] = '\0';

        p = strtok(NULL, "|");
    }
    to_split[poz] = '\0';
    return to_split;
}

int shell_execution_pipes(char ***cmd)
{
    int p[2];
    pid_t pid;
    int fd_in = 0;
    while (*cmd != NULL)
    {
        pipe(p);
        if ((pid = fork()) == -1)
        {
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            dup2(fd_in, STDIN_FILENO); // change the input according to the old one
            if (*(cmd + 1) != NULL)
                dup2(p[WRITE], STDOUT_FILENO);
            close(p[READ]);
            // printf("%s ", *cmd[0]);
            execvp((*cmd)[0], *cmd);
            exit(EXIT_FAILURE);
        }
        else
        {
            wait(NULL);
            close(p[1]);
            fd_in = p[READ]; // save the input for the next command
            cmd++;
        }
    }

    return 1;
}

void shell_loop()
{
    char *line;
    char *copy_line;
    char **pipe_args;
    int status;

    do
    {
        char **args;
        current_directory();
        line = shell_read_line();
        pipe_args = shell_split_pipes(line);
        if (pipe_number == 0)
        {
            copy_line = shell_redirect(line);
            args = shell_split_line(copy_line);
            status = shell_execution(args);
        }
        else
        {
            // printf("pipe_number %d\n", pipe_number);

            // for (int i = 0; i <= pipe_number; i++)
            // {
            //     printf("%s ", pipe_args[i]);
            //     printf("%ld\n", strlen(pipe_args[i]));
            // }

            // pipe_args_separation=shell_split_line(*pipe_args);

            status = shell_execution_pipes((char ***)pipe_args);

            // char *ls[] = {"ls", NULL};
            // char *grep[] = {"grep", "pipe", NULL};
            // char *wc[] = {"wc", NULL};
            // char **cmd[] = {ls, grep, wc, NULL};
            // status = shell_execution_pipes(cmd);
        }

        bzero(line, sizeof(line));
        // bzero(args, sizeof(args));
        has_output = 0;
        has_input = 0;
        pipe_number = -1;
    } while (status);
}
int main(int argc, char **argv)
{
    shell_loop();
    return 0;
}