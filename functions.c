#include "functions.h"
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void handle_error_soft(const char *msg)
{
    fprintf(stderr, "[client]: %s. ", msg);
    fflush(stdout);
    if (errno != 0)
    {
        perror("Reason");
    }
    else
    {
        printf("\n");
    }
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

SSL_CTX *InitServerCTX(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();     /* load & register all cryptos, etc. */
    SSL_load_error_strings();         /* load all error messages */
    method = TLS_server_method(); /* create new server-method instance */
    ctx = SSL_CTX_new(method);        /* create new context from method */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void LoadCertificates(SSL_CTX *ctx, char *CertFile, char *KeyFile)
{
    /* set the local certificate from CertFile */
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void ShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if (cert != NULL)
    {
        printf("Server certificates:\n");
        if ((line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0)) == NULL)
            handle_error_soft("unable to get subject name of certificate");
        else
            printf("Subject: %s\n", line);
        free(line);
        if ((line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0)) == NULL)
            handle_error_soft("unable to get issuer name of certificate");
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
    OpenSSL_add_all_algorithms();     /* Load cryptos, et.al. */
    SSL_load_error_strings();         /* Bring in and register error messages */
    method = TLS_client_method(); /* Create new client-method instance */
    ctx = SSL_CTX_new(method);        /* Create new context */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
