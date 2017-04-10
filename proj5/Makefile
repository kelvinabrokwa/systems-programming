FLAGS = -g -Wall
LIBS = -lpthread

all: counter.o
	gcc counter.o $(FLAGS) $(LIBS) -o counter
counter.o:
	gcc $(FLAGS) -c counter.c
clean:
	rm -f counter.o counter
run:
	./counter -b 4 -t 6 -d 0 -D 0 /usr/share/fortune/linuxcookie
run-1-counter:
	./counter -b 4 -t 1 -d 0 -D 0 /usr/share/fortune/linuxcookie
run-1-slot:
	./counter -b 1 -t 1 -d 0 -D 0 /usr/share/fortune/linuxcookie
.PHONY: clean
