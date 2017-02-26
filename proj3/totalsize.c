/**
 * totalsize.c
 * Author: Kelvin Abrokwa-Johnson
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void print_usage() {
    fprintf(stderr, "This is totalsize usage\n");
}

int main() {
    char* endptr;

    // parse TSTALL env var
    char* tstall_str = getenv("TSTALL");
    long tstall = 0;
    if (tstall_str != NULL) {
        tstall = strtol(tstall_str, &endptr, 10);
        if (errno != 0 || *endptr != '\0')
            tstall = 0;
    }

    // parse UNITS env var
    char* units = getenv("UNITS");
    int in_kb = 0;
    if (units != NULL) {
        if (*units == 'k' || *units == 'K') // FIXME: error on kbogus
            in_kb = 1;
    }

    // parse TMOM env var
    char* tmom_str = getenv("TMOM");
    long tmom = 0;
    if (tmom_str != NULL) {
        tmom = strtol(tmom_str, &endptr, 10);
        if (errno != 0 || *endptr != '\0' || tmom <= 0)
            tmom = 0;
    }
    
    char filename[4096];
    long totalsize  = 0;
    struct stat sb;
    while (scanf("%s", filename) != EOF) {
        if (tstall) 
            sleep(tstall);
        if (stat(filename, &sb) == -1)
            continue;
        totalsize += sb.st_size;
    }

    if (in_kb) {
        printf("%ldkB\n", totalsize / 1000);
    } else {
        printf("%ld\n", totalsize);
    }

    if (tmom) {
        kill(tmom, SIGUSR1);
    }

    return 0;
}

