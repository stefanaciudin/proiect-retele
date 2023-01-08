CC = gcc
SERVER_LIB = -L/usr/lib -lssl -lcrypto -lsqlite3 -lcrypt
CLIENT_LIB = -L/usr/lib -lssl -lcrypto
OPTIONS = -Wall
SERVER_SOURCES = server.c src/functions.c src/errors.c src/database_functions.c
CLIENT_SOURCES = client.c src/functions.c src/errors.c
SERVER_BIN = server
CLIENT_BIN = client
all:
	${CC} -o ${SERVER_BIN} ${OPTIONS} ${SERVER_SOURCES} ${SERVER_LIB}
	${CC} -o ${CLIENT_BIN} ${OPTIONS} ${CLIENT_SOURCES} ${CLIENT_LIB}
	
client: client.c
	${CC} -o ${CLIENT_BIN} ${OPTIONS} ${CLIENT_SOURCES} ${CLIENT_LIB}
server: server.c
	${CC} -o ${SERVER_BIN} ${OPTIONS} ${SERVER_SOURCES} ${SERVER_LIB}
clean :
	rm -f server client projectdb.db