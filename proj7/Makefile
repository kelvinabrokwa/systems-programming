CC=gcc
CFLAGS=-g -Wall

all:	TTT tt

TTT:	TTT.o msg.o
	gcc -o TTT TTT.o msg.o

tt:	tt.o msg.o
	gcc -o tt tt.o msg.o

msg.o:	msg.c common.h

TTT.o:	TTT.c common.h

tt.o:	tt.c common.h

clean:
	rm -f *.o tt TTT serverloc
