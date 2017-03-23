FLAGS = -D DEBUG

all:
	make server && make client
server: server.o msg.o
	gcc server.o msg.o $(FLAGS) -o server
client: client.o msg.o
	gcc client.o msg.o $(FLAGS) -o client
server.o: server.c
	gcc $(FLAGS) -c server.c
client.o: client.c
	gcc -c client.c
msg.o: msg.c
	gcc -c msg.c
clean:
	rm -r server.o client.o msg.o server client
.PHONY: clean
