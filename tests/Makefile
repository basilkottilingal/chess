# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra 
#CFLAGS += -std=c11 -Wconversion

libnnue.a:
	cd ../src/nnue && make

#compile the main webserver file (wsserver.c);
%.t: libnnue.a %.c
	gcc -std=c99 -o run $*.c -I ../src/nnue -L ../src/nnue -lnnue -lm
