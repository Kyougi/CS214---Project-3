COMPILER     = gcc
CFLAGS = -g -Wall -Wimplicit-function-declaration
CFLAGS2 = -g -Wall -pthread -Wimplicit-function-declaration
all: ttts ttt

ttts: ttts.o
	$(COMPILER) $(CFLAGS2) -o ttts ttts.o
ttt: ttt.o
	$(COMPILER) $(CFLAGS) -o ttt ttt.o
ttts.o: ttts.c
	$(COMPILER) $(CFLAGS2) -c ttts.c
ttt.o: ttt.c
	$(COMPILER) $(CFLAGS) -c ttt.c

clean:
	rm -f *.o ttts ttt 