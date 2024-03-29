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

#include "src/functions.h"
#include "src/errors.h"

#define PORT 2424

int create_connection(char *address, int port)
{
    int sd;
    struct sockaddr_in server;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        handle_error_exit("[client] - socket");
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(address);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        if (close(sd) == -1)
            perror("[client] error - close");
        handle_error_exit("[client] - connect");
    }
    return sd;
}

int main(int argc, char *argv[])
{
    SSL_CTX *ctx;
    SSL *ssl;
    int server;

    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }
    int port = atoi(argv[2]);

    char command[MAX_COMMAND]; // stores the command given by the user
    command[0] = '\0';
    int n;

    SSL_library_init();
    ctx = InitCTX();
    server = create_connection(argv[1], port);
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server);
    if (SSL_connect(ssl) == -1) // perform the connection
        ERR_print_errors_fp(stderr);
    else
    {
        ShowCerts(ssl);                        // print certificates
        while (strcmp("exit\n", command) != 0) // exit stops the execution
        {
            n = read(0, command, MAX_COMMAND); // reading the command
            command[n] = '\0';
            SSL_write(ssl, command, n); // writing the command in the socket descriptor
            char answer[MAX_ANSWER];
            SSL_read(ssl, answer, MAX_ANSWER); // reading the answer from the socket descriptor
            answer[strlen(answer)] = '\0';
            printf("%s\n", answer);
        }
    }
    SSL_free(ssl);
    if (close(server) == -1)
        perror("[client] error - close");
    SSL_CTX_free(ctx);
}
