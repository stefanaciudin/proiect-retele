all:
	gcc -Wall -o server server.c functions.c errors.c database_functions.c -L/usr/lib -lssl -lcrypto -lsqlite3 -lcrypt
	gcc -Wall -o client  client.c functions.c errors.c -L/usr/lib -lssl -lcrypto
client: client.c
	gcc -Wall -o client  client.c functions.c errors.c -L/usr/lib -lssl -lcrypto
server: server.c
	gcc -Wall -o server server.c functions.c errors.c database_functions.c -L/usr/lib -lssl -lcrypto -lsqlite3 -lcrypt
clean :
	rm -f server client test projectdb.db