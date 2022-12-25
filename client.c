#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 2424
#define MAX_COMMAND 2000
#define MAX_ANSWER 3000

int create_connection()
{
    int sd;
    struct sockaddr_in server;
    char address[] = "127.0.0.1";

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[client] error - socket \n");
        return 1;
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(address);
    server.sin_port = htons(PORT);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        if (close(sd) == -1)
            perror("[client] error - close");
        perror("[client] error - connect \n");
        return 1;
    }
    return sd;
}
SSL_CTX *InitCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();     /* Load cryptos, et.al. */
    SSL_load_error_strings();         /* Bring in and register error messages */
    method = TLSv1_2_client_method(); /* Create new client-method instance */
    ctx = SSL_CTX_new(method);        /* Create new context */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void ShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if (cert != NULL)
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line); /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);      /* free the malloc'ed string */
        X509_free(cert); /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

int main()
{
    SSL_CTX *ctx;
    SSL *ssl;
    int server;

    char command[MAX_COMMAND]; // stores the command given by the user
    command[0] = '\0';
    int n;

    SSL_library_init();
    ctx = InitCTX();
    server = create_connection();
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server);
    if (SSL_connect(ssl) == -1) /* perform the connection */
        ERR_print_errors_fp(stderr);
    else
    {
        while (strcmp("exit\n", command) != 0) // exit stops the execution
        {
            // current_directory();
            // printf(">>");
            n = read(0, command, MAX_COMMAND); // reading the command
            command[n] = '\0';

            ShowCerts(ssl);

            SSL_write(ssl, command, n); // writing the command in the socket descriptor
            char answer[MAX_ANSWER];
            SSL_read(ssl, answer, MAX_ANSWER); // reading the answer from the socket descriptor
            answer[strlen(answer)] = '\0';
            printf("%s\n", answer);
        }
    }
    SSL_free(ssl);
    if(close(server)==-1)
        perror("[client] error - close");
    SSL_CTX_free(ctx);
}
