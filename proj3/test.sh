#!/usr/bin/env bash
set -x

make

separator() {
    set +x
    printf "\n\n\n----------------------------------------------------\n\n\n"
    set -x
}

printf "Testing\n"

export UNITS=
export TMOM=
export TSTALL=

separator
ls | ./totalsize
separator
(export UNITS=k && ls | ./totalsize)
separator
(export UNITS=K && ls | ./totalsize)
separator
(export UNITS=bogus && ls | ./totalsize)
separator
(export TMOM=-1 && ls | ./totalsize)
separator
(export TMOM=10000000 && ls | ./totalsize)
separator
(export TMOM=0 && ls | ./totalsize)
separator
(export TSTALL=0 && ls | ./totalsize)
separator
(export TSTALL=-1 && ls | ./totalsize)
separator
(export TSTALL=1 && ls | ./totalsize)
separator
(export UNITS=k && export TMOM=-1 && export TSTALL=1 && ls | ./totalsize)
separator

ls | ./accessed 1
separator
ls | ./accessed -1
separator
ls | ./accessed 100000000000
separator
ls | ./accessed -100000000000
separator
ls | ./accessed bogus
separator
ls | ./accessed -bogus
separator

ls | ./report 1
separator
ls | ./report 1 -d 1
separator
ls | ./report 1 -d -1
separator
ls | ./report 1 -d bogus
separator

ls | ./report 1 -k
separator
ls | ./report 1 -K
separator
ls | ./report 1 -Kill
separator

ls | ./report 1 -d 1 -k
separator
ls | ./report 1 -k -d 1
separator
ls | ./report 1 -d -k
separator
ls | ./report 1 -k -d
separator

(cd pgm3test.d/ && ./script)
find ./pgm3test.d/subdir/subsubdir -print | ./totalsize
separator

(cd pgm3test.d/ && ./script)
find ./pgm3test.d -print | ./totalsize
separator

(cd pgm3test.d/ && ./script)
find ./pgm3test.d/subdir/subsubdir -print | ./accessed 5
separator

(cd pgm3test.d/ && ./script)
find ./pgm3test.d -print | ./report 5
separator

./report 5
separator

