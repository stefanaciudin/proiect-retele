#include <stdio.h>
#include <errno.h>


void handle_error(char *msg); //exit(1)
int handle_error_ret(char *msg); //used in server - returns 0 in case of error
void handle_error_soft(const char *msg); //just prints the error