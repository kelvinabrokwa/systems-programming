#!/usr/bin/env bash

make

printf "\n--------------------normal word mode-----------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int 

printf "\n--------------------normal word mode with -b--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int -b

printf "\n--------------------normal word mode with -n--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int -n

printf "\n--------------------normal word mode with -n -b--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int -n -b

printf "\n--------------------normal word mode with -b -n--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int -b -n

printf "\n--------------------normal word mode with -nb--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int -nb

printf "\n--------------------normal word mode with -bn--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w int -bn

printf "\n--------------------word mode without word argument--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w

printf "\n--------------------Mismatach between grep'ed word and rgpp'ed word--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w sup -n

printf "\n--------------------normal line mode--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -l

printf "\n--------------------normal line mode with -n--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -l -n

printf "\n--------------------normal line mode with -b--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -l -b

printf "\n---------------------w and -l--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -w -l

printf "\n---------------------l and -w--------------------\n\n"
grep -r -H -n -s -I -i int rgpp.c Makefile | ./rgpp -l -w

