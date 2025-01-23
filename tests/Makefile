# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Target executable
TARGET = program

SRC = ../src
NNUE = ../src/nnue
OBJ = ../obj

# Dependencies for nnue eval 
nnue = $(NNUE)/nnue.cpp $(NNUE)/misc.cpp

# make an execuatable.
# example
#  $ make move.tst && ./run 
%.tst: %.c 
	$(CC) $(CFLAGS) $< $(nnue) -o run -lm
