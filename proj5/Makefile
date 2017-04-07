FLAGS = -g -Wall
LIBS = -lpthread

all: counter.o
	gcc counter.o $(FLAGS) $(LIBS) -o counter
counter.o:
	gcc $(FLAGS) -c counter.c
clean:
	rm -f counter.o counter
.PHONY: clean
