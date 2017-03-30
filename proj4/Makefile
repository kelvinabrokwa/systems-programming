FLAGS = -g -Wall

all:
	make server && make client
server: server.o msg.o
	gcc server.o msg.o $(FLAGS) -o TTT
client: client.o msg.o
	gcc client.o msg.o $(FLAGS) -o ttt
server.o: server.c
	gcc $(FLAGS) -c server.c
client.o: client.c
	gcc $(FLAGS) -c client.c
msg.o: msg.c
	gcc $(FLAGS) -c msg.c
clean:
	rm -f server.o client.o msg.o TTT ttt server_addr.txt
.PHONY: clean
