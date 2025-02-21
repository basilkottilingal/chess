#Webscoket Library
libws.a: 
	cd wsServer && make

#NNUE library
nnue.a:
	cd nnue && make

#compile the main webserver file (wsserver.c);
server: libws.a nnue.a
	gcc -std=c99 -o wsserver wsserver.c -I./nnue -I./wsServer/include -L./nnue -L./wsServer -lnnue -lws -pthread -lm -DDISABLE_VERBOSE

all: server
