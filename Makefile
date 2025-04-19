all:
	gcc -g  ./src/servidor.c -o ./bin/servidor -lmysqlclient -lncurses
	gcc -g ./src/cliente.c -o ./bin/cliente -lncurses

