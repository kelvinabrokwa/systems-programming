all:
	gcc -g -o rgpp rgpp.c -Wall
run:
	make all && grep -r -H -n -s -I -i int . | ./rgpp -l -b -n
