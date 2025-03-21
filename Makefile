all:
	gcc ./src/servidor.c -o ./bin/servidor -lmysqlclient
	gcc ./src/cliente.c -o ./bin/cliente -lncurses

