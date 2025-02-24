#Webscoket Library
libws.a: 
	cd wsServer && make

#NNUE library
nnue.a:
	cd nnue && make

SRC=$(wildcard:*.c);

#compile the main webserver file (wsserver.c);
server: libws.a nnue.a $(SRC)
	gcc -std=c99 -o server wsserver.c -I./nnue -I./wsServer/include -L./nnue -L./wsServer -lnnue -lws -pthread -lm -DDISABLE_VERBOSE

all: server
