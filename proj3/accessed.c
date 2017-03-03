/**
 * accessed.c
 * Author: Kelvin Abrokwa-Johnson
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>

#define SECONDS_IN_DAY 86400

void print_usage() {
    fprintf(stderr, "\nUsage: ls | accessed <num>\n"
                    "\naccessed takes a list of files on stdin and prints those\n"
                    "that have not been accessed in num days\n"
                    "If num is negative, accessed prints the files\n"
                    "that have been accessed within num days\n");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Not enough arguments\n");
        print_usage();
        exit(1);
    }

    char* endptr;
    long num = strtol(argv[1], &endptr, 10);

    if (errno != 0 || *endptr != '\0' || num == 0) {
        fprintf(stderr, "Incorrect num argument\n");
        perror("strtol");
        print_usage();
        exit(1);
    }

    time_t curr_time;
    time(&curr_time);

    if (errno != 0 || curr_time == -1) {
        fprintf(stderr, "Time failure\n");
        perror("time");
        print_usage();
        exit(1);
    }

    char filename[1024];
    struct stat sb;

    time_t days_in_secs = abs(num) * SECONDS_IN_DAY;

    while (scanf("%s", filename) != EOF) {
        if (stat(filename, &sb) == -1)
            continue;
        if (!S_ISREG(sb.st_mode))
            continue;
        if (num > 0) { // files which have not been accessed in days
            if ((curr_time - sb.st_atime) > days_in_secs)
                printf("%s\n", filename);
        } else { // files which have been accessed in days
            if ((curr_time - sb.st_atime) <= days_in_secs)
                printf("%s\n", filename);
        }
    }

    return 0;
}
