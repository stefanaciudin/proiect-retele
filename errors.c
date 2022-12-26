#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "errors.h"

void handle_error_soft(const char *msg) //used in client
{
    fprintf(stderr, "[client]: %s. ", msg);
    fflush(stdout);
    if (errno != 0)
        perror("Reason");
    else
        printf("\n");
}
void handle_error(char *msg)
{
    fprintf(stderr, "[server] error - %s\n", msg);
    fflush(stdout);
    if (errno)
        perror("Reason");
    printf("\n");
    exit(1);
}
int handle_error_ret(char *msg)
{
    fprintf(stderr, "[server] error - %s\n", msg);
    fflush(stdout);
    if (errno)
        perror("Reason");
    printf("\n");
    return 0;
}

