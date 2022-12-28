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
void create_account(char *user, char *pass)
{

    int rc;
    sqlite3_stmt *stmt1;
    sqlite3_prepare_v2(db, "SELECT username, password FROM users WHERE username = ?1;", -1, &stmt1, NULL);
    sqlite3_bind_text(stmt1, 1, user, -1, SQLITE_STATIC);

    if ((rc = sqlite3_step(stmt1)) == SQLITE_ROW)
    {
        printf("Acest utilizator exista deja!\n");
    }
    else
    {
        printf("Username: %s, Password: %s", user, pass);
        sqlite3_stmt *stmt2;
        sqlite3_prepare_v2(db, "INSERT INTO USERS VALUES(?1, ?2);", -1, &stmt2, NULL);
        sqlite3_bind_text(stmt2, 1, user, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt2, 2, pass, -1, SQLITE_STATIC);
        sqlite3_step(stmt2);

        printf("Utilizator creat cu succes.\n");
    }
}

void login(char *user, char *pass)
{

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT username, password FROM users WHERE username = ?1;", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);

    int was_made = 0;
    int rc;
    int ok = 0;
    const char *right_pass;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const char *value = sqlite3_column_text(stmt, 0);
        right_pass = sqlite3_column_text(stmt, 1);
        if (strcmp(value, user) == 0)
        {
            ok = 1;
            break;
        }
    }

    if (!ok)
    {
        printf("Nu exista cont cu acest user\n");

        printf("username inexistent -- create new account? y/n\n");
        char ans[3];
        scanf("%s", ans);

        if (strcmp(ans, "y") == 0)
        {
            create_account(user, pass);
            was_made++;
        }
    }
    {
        int verif = 0;
        if (strcmp(right_pass, pass) == 0)
            verif++;
        if (verif)
        {
            printf("Te ai conectat cu succes\n");
        }
        else if(!was_made)
            printf("Wrong pass\n");
    }
}

int main()
{
    char username[20], password[30];

    read(0, username, 20);
    username[sizeof(username) - 1] = '\0';

    read(0, password, 30);
    password[sizeof(password) - 1] = '\0';

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

    login(username, password);
}