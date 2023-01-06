#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>


sqlite3 *db;
char *zErrMsg;
int dbproject;

int create_account(char *username, char *password)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("projectdb.db", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_prepare_v2(db, "INSERT INTO users (username, password) VALUES (?, ?)", -1, &stmt, 0);
    if (rc)
    {
        fprintf(stderr, "Can't prepare INSERT statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    if (rc)
    {
        fprintf(stderr, "Can't bind username: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    if (rc)
    {
        fprintf(stderr, "Can't bind password: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Error inserting row: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

int login(char *username, char *password)
{
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(db, "SELECT password FROM users WHERE username = ?", -1, &stmt, 0);
    if (rc)
    {
        fprintf(stderr, "Can't prepare SELECT statement: %s\n", sqlite3_errmsg(db));
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
        printf("The username does not exist. Create a new account? [y/n]\n");
        char ans[3];
        scanf("%s", ans);
        ans[sizeof(ans)] = '\0';
        if (strcmp(ans, "y") == 0)
        {
            if (create_account(username, password))
            {
                printf("Account created successfully.\n");
                return 1;
            }
         
        }
    }
    else
    {
        fprintf(stderr, "Error executing SELECT statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int main()
{
    dbproject = sqlite3_open("projectdb.db", &db);
    if (dbproject != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    // creare tabel
    char *sql1;
    sql1 = "CREATE TABLE USERS("
           "USERNAME TEXT PRIMARY KEY NOT NULL, "
           "PASSWORD TEXT             NOT NULL);";
    // execute
    dbproject = sqlite3_exec(db, sql1, NULL, 0, &zErrMsg);

    while (1)
    {
        char username[20], password[30];

        read(0, username, 20);
        username[sizeof(username) - 1] = '\0';

        read(0, password, 30);
        password[sizeof(password) - 1] = '\0';
        if (login(username, password))
            printf("Login successful\n");
        else
            printf("Login unsuccessful\n");
    }
}