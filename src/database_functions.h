#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sqlite3.h>

int insert_client(int pid);                         // inserts the pid in the database
int get_logged_status();                            // returns the logged status for the current client
int update_logged_status();                         // updates the logged status for the current client
int create_account(char *username, char *password); // creates account and inserts data in the database