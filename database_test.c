#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sqlite3.h>

#include <crypt.h>
#define SALT "$5$bpKU3bUSQLwX87z/$"

char *zErrMsg;
int dbproject;

#define SALT_LENGTH 20

char *generate_salt()
{
    char *salt = malloc(SALT_LENGTH + 1);
    if (salt == NULL)
    {
        perror("malloc");
        return NULL;
    }

    // Initialize the random number generator with the current time
    srand(time(NULL));
    salt[0] = '$';
    salt[1] = '5';
    salt[2] = '$';
    for (int i = 3; i < SALT_LENGTH - 2; i++)
    {
        // Generate a random printable ASCII character
        salt[i] = 48 + rand() % 10;
    }
    salt[SALT_LENGTH - 2] = '/';
    salt[SALT_LENGTH - 1] = '$';
    salt[SALT_LENGTH] = '\0';

    return salt;
}

int create_account(char *username, char *password)
{
    // Hash the password using the salt
    char *salt = generate_salt();
    char *hash = crypt(password, salt);

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_open("projectdb.db", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_prepare_v2(db, "INSERT INTO users (username, password, salt) VALUES (?, ?, ?)", -1, &stmt, 0);
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

    rc = sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_STATIC);
    if (rc)
    {
        fprintf(stderr, "Can't bind password: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_STATIC);
    if (rc)
    {
        fprintf(stderr, "Can't bind salt: %s\n", sqlite3_errmsg(db));
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
    sqlite3 *db;
    rc = sqlite3_open("projectdb.db", &db);

    rc = sqlite3_prepare_v2(db, "SELECT password, salt FROM users WHERE username = ?", -1, &stmt, 0);
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
        char *stored_hash = (char *)sqlite3_column_text(stmt, 0);
        char *salt = (char *)sqlite3_column_text(stmt, 1);
        char *inputted_hash = crypt(password, salt);
        if (strcmp(inputted_hash, stored_hash) == 0)
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
    sqlite3 *db;
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
           "PASSWORD TEXT             NOT NULL, "
           "SALT     TEXT             NOT NULL);";
    // execute
    dbproject = sqlite3_exec(db, sql1, NULL, 0, &zErrMsg);

    while (1)
    {

        char username[20], password[30];
        bzero(username, 20);
        bzero(password, 30);
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