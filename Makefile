all:
	gcc ./src/servidor.c -o ./bin/servidor -lmysqlclient
	gcc ./src/cliente.c -o ./bin/cliente
	gcc .//src/pruebas2.c -o ./bin/pruebas2 -lpthread -lncurses

