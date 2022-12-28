all:
	gcc -Wall -o server server.c functions.c errors.c -L/usr/lib -lssl -lcrypto
	gcc -Wall -o client  client.c functions.c errors.c -L/usr/lib -lssl -lcrypto
client: client.c
	gcc -Wall -o client  client.c functions.c errors.c -L/usr/lib -lssl -lcrypto
server: server.c
	gcc -Wall -o server server.c functions.c errors.c -L/usr/lib -lssl -lcrypto

database : database_test.c
	gcc database_test.c -o test -lsqlite3

clean :
	rm -f server client test projectdb.db