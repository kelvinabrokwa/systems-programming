CFLAGS = -Wall

all:
	make totalsize && make accessed && make report

totalsize:
	gcc $(CFLAGS) totalsize.c -o totalsize

accessed:
	gcc $(CFLAGS) accessed.c -o accessed

report:
	gcc $(CFLAGS) report.c -o report

.PHONY: accessed totalsize report

