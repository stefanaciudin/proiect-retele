#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sqlite3.h>

#include "database_functions.h"
#include "functions.h"
#include "server.h"

int get_logged_status() // gets the logged status of the current client
{
    sqlite3 *db;
    int rc;
    sqlite3_stmt *stmt;

    int pid = getpid();
    rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_prepare_v2(db, "SELECT logged_status FROM login WHERE pid = ?", -1, &stmt, 0);
    if (rc)
    {
        fprintf(stderr, "Can't prepare SELECT statement get-logged: %s %d\n", sqlite3_errmsg(db), sqlite3_extended_errcode(db));
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 1, pid);
    if (rc)
    {
        fprintf(stderr, "Can't bind pid: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        int stored_logged_status = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return stored_logged_status;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

int update_logged_status()
{
    sqlite3 *db;
    int rc;
    sqlite3_stmt *stmt;

    int pid = getpid();
    rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    int check = get_logged_status();
    if (!check)
    {
        rc = sqlite3_prepare_v2(db, "UPDATE login SET logged_status=1 WHERE pid = ?", -1, &stmt, 0);
        if (rc)
        {
            fprintf(stderr, "Can't prepare UPDATE statement: %s %d\n", sqlite3_errmsg(db), sqlite3_extended_errcode(db));
            return 0;
        }
    }
    else
    {
        rc = sqlite3_prepare_v2(db, "UPDATE login SET logged_status=0 WHERE pid = ?", -1, &stmt, 0);
        if (rc)
        {
            fprintf(stderr, "Can't prepare UPDATE statement: %s %d\n", sqlite3_errmsg(db), sqlite3_extended_errcode(db));
            return 0;
        }
    }

    rc = sqlite3_bind_int(stmt, 1, pid);
    if (rc)
    {
        fprintf(stderr, "Can't bind pid: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Can't execute UPDATE statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

int create_account(char *username, char *password) // creates account and inserts data in the database
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    char *salt = generate_salt();
    char *hash = crypt(password, salt);
    rc = sqlite3_open(path, &db);
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

int insert_client(int pid)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    int is_logged = 0;
    rc = sqlite3_open(path, &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_prepare_v2(db, "INSERT INTO LOGIN (pid, logged_status) VALUES (?, ?)", -1, &stmt, 0);
    if (rc)
    {
        fprintf(stderr, "Can't prepare INSERT statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    rc = sqlite3_bind_int(stmt, 1, pid);
    if (rc)
    {
        fprintf(stderr, "Can't bind pid: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_bind_int(stmt, 2, is_logged);
    if (rc)
    {
        fprintf(stderr, "Can't bind logged status: %s\n", sqlite3_errmsg(db));
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