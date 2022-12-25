all:
	gcc -Wall -o server server.c -L/usr/lib -lssl -lcrypto
	gcc -Wall -o client  client.c -L/usr/lib -lssl -lcrypto
client:
	gcc -Wall -o client  client.c -L/usr/lib -lssl -lcrypto
server:
	gcc -Wall -o server server.c -L/usr/lib -lssl -lcrypto
