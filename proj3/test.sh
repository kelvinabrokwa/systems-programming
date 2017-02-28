#!/usr/bin/env bash
set -x

make report
make totalsize
make accessed

separator() {
    set +x
    printf "\n\n\n----------------------------------------------------\n\n\n"
    set -x
}

printf "Testing\n"

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

find ./pgm3test.d/subdir/subsubdir -print | ./totalsize
separator

find ./pgm3test.d -print | ./totalsize
separator

find ./pgm3test.d/subdir/subsubdir -print | ./accessed 5
separator

find ./pgm3test.d -print | ./report 5
separator

./report 5

