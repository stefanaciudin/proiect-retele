#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define PORT 2424

#define READ 0
#define WRITE 1

#define MAX_COMMAND 2000 // max number of letters in a command
#define MAX_ANSWER 3000 //max number of letters in answer

#define BUFF_SIZE 1024
#define DELIM " \n"

void handle_error(char *msg); //exit(1)
int handle_error_ret(char *msg); //used in server - returns 0 in case of error
void handle_error_soft(const char *msg); //just prints the error

SSL_CTX *InitServerCTX(void);
void LoadCertificates(SSL_CTX *ctx, char *CertFile, char *KeyFile);
void ShowCerts(SSL *ssl);
SSL_CTX *InitCTX(void);