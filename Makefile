all:
	gcc -Wall -o server server.c functions.c errors.c -L/usr/lib -lssl -lcrypto
	gcc -Wall -o client  client.c functions.c errors.c -L/usr/lib -lssl -lcrypto
client: client.c
	gcc -Wall -o client  client.c functions.c errors.c -L/usr/lib -lssl -lcrypto
server: server.c
	gcc -Wall -o server server.c functions.c errors.c -L/usr/lib -lssl -lcrypto
clean :
	rm server
	rm client