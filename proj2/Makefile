all:
	gcc -g -o rgpp rgpp.c -Wall
run:
	make all && grep -r -H -n -s -I -i int . | ./rgpp -w int -b -n
test:
	./test.sh
