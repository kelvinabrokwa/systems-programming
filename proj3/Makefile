CFLAGS = -Wall -g

all:
	make totalsize && make accessed && make report

totalsize:
	gcc $(CFLAGS) totalsize.c -o totalsize

accessed:
	gcc $(CFLAGS) accessed.c -o accessed

report:
	gcc $(CFLAGS) report.c -o report

test:
	./test.sh

top:
	top -U kelvin

valgrind:
	valgrind --leak-check=yes find ./pgm3test.d -print | ./report 5

.PHONY: accessed totalsize report

