libws.a: 
	cd wsServer && make

#compile the main webserver file (wsserver.c);
server: libws.a
	gcc -std=c99 -o wsserver wsserver.c -I./wsServer/include -L./wsServer -lws -pthread -lm -DDISABLE_VERBOSE
