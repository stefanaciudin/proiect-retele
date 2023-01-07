#include "functions.h"
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "functions.h"
#include "server.h"


char *generate_salt()
{
    char *salt = malloc(SALT_LENGTH + 1);
    if (salt == NULL)
    {
        perror("malloc");
        return NULL;
    }

    srand(time(NULL));
    salt[0] = '$';
    salt[1] = '5';
    salt[2] = '$';
    for (int i = 3; i < SALT_LENGTH - 2; i++)
    {
        // generate a random number
        salt[i] = 48 + rand() % 10;
    }
    salt[SALT_LENGTH - 2] = '/';
    salt[SALT_LENGTH - 1] = '$';
    salt[SALT_LENGTH] = '\0';

    return salt;
}

void LoadCertificates(SSL_CTX *ctx, char *CertFile, char *KeyFile)
{

    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0) // set the local certificate from CertFile
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0) // set private key from KeyFile
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    if (!SSL_CTX_check_private_key(ctx)) // verify private key
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void ShowCerts(SSL *ssl) // used in client
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); // get certificates
    if (cert != NULL)
    {
        printf("Server certificates:\n");
        if ((line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0)) == NULL)
            handle_error("unable to get subject name of certificate");
        else
            printf("Subject: %s\n", line);
        free(line);
        if ((line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0)) == NULL)
            handle_error("unable to get issuer name of certificate");
        else
            printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates available\n");
}

SSL_CTX *InitCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();     // load error messages
    method = TLS_client_method(); // create new client-method instance
    ctx = SSL_CTX_new(method);    // create new context
    if (ctx == NULL)              // print errors
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

SSL_CTX *InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();     // load error messages
    method = TLS_server_method(); // create new server-method instance
    ctx = SSL_CTX_new(method);    // create new context from method
    if (ctx == NULL)              // print any existing errors
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

char *get_path()
{
    char *to_return = malloc(sizeof(char) * 500);
    char path[500];
    int len = 500;
    if (readlink("/proc/self/exe", path, len) != -1)
    {
        dirname(path);
        strcat(path, "/projectdb.db");
    }
    strcpy(to_return, path);
    return to_return;
}