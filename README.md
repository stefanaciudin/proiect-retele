# MySSH - proiect retele :)
Cerință - Sa se implementeze o pereche client/server capabila de autentificare si comunicare encriptate. Server-ul va executa comenzile de la client, si va returna output-ul lor clientului. Comenzile sunt executabile din path, cu oricat de multe argumente; cd si pwd vor functiona normal. Se pot executa comenzi multiple legate intre ele sau redirectate prin: |, <, >, 2>, &&, ||, ;.

## Extras needed:
OpenSSL instalation:
> ```shell
> sudo apt-get install libssl–dev
> ```
SQLite:
> ```shell
> sudo apt install libsqlite3-dev
> ```
Key generation:
> ```shell
> openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout mycert.pem -out mycert.pem
> ```
