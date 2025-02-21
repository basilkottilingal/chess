all: libnnue.a

#object file
%.o: %.cpp
	gcc $< -Wall -Wextra -c -o $@

#archiving object files int single static library file
libnnue.a: nnue.o misc.o
	ar -cr libnnue.a $^ 
