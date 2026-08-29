# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra 
#CFLAGS += -std=c11 -Wconversion

libnnue.a:
	cd ../src/nnue && make

#compile the main webserver file (wsserver.c);
%.s: libnnue.a %.c
	gcc -std=c99 -o run $*.c -I ../src/nnue -L ../src/nnue -lnnue -lm

%.tst: %.s
	./run

all: board.tst move.tst tree.tst probe.tst
