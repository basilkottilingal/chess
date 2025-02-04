libws.a: 
	cd wsServer && make

server: libws.a
	gcc -std=c99 -o wsserver wsserver.c -I./wsServer/include -L./wsServer -lws -pthread 
