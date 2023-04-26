COMPILER     = gcc
CFLAGS2 = -g -Wall -pthread -Wimplicit-function-declaration
all: ttts1

ttts1: ttts1.o
	$(COMPILER) $(CFLAGS2) -o ttts1 ttts1.o

ttts1.o: ttts1.c
	$(COMPILER) $(CFLAGS2) -c ttts1.c

clean:
	rm -f *.o ttts1