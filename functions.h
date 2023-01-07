#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

#include "errors.h"
#include "server.h"

#define PORT 2424

#define READ 0
#define WRITE 1

#define MAX_COMMAND 2000 // max number of letters in a command
#define MAX_ANSWER 3000  // max number of letters in answer

#define BUFF_SIZE 1024
#define DELIM " \n"

#define SALT_LENGTH 20

char *get_path(); // gets the path of the db file

char *generate_salt(); // generates salt for crypt() in server
SSL_CTX *InitServerCTX(void);
void LoadCertificates(SSL_CTX *ctx, char *CertFile, char *KeyFile);
void ShowCerts(SSL *ssl);
SSL_CTX *InitCTX(void);
