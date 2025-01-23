# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Target executable
TARGET = program

# Source files and object files
SRC = main.c file1.c file2.c
OBJ = $(SRC:.c=.o)

# Build the target
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.tst: %.o
  $(CC) $(CFLAGS) 

# Clean up build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

