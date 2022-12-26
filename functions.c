#include "functions.h"
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "functions.h"

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