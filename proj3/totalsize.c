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
    fprintf(stderr, "Usage: ls | totalsize\n"
                    "totalsize takes a white space separated list of files on\n"
                    "its standard input and prints out total size of those it can\n"
                    "access\n"
                    "Environment Variables:\n"
                    "  UNITS       When UNITS=k or UNITS=K the output is in kilobytes\n"
                    "  TSTALL      When TSTALL=n totalsize waits n seconds between"
                                   " processing inputs\n"
                    "  TMOM\n      When TMOM=pid is a valid process id, totalsize signals\n"
                    "              that process when it has tallied the total size\n");
}

struct Stat {
    dev_t st_dev;
    ino_t st_ino;
    struct Stat* next;
};

/**
 * return 0 on success and -1 on error
 */
int insert(struct Stat** head, struct stat* sb) {
    struct Stat* s = (struct Stat*)malloc(sizeof(struct Stat));
    if (s == NULL)
        return -1;
    s->st_dev = sb->st_dev;
    s->st_ino = sb->st_ino;
    s->next = *head;
    *head = s;
    return 0;
}

/**
 * returns 1 when the list contains the node and 0 if it doesn't
 */
int contains(struct Stat* head, struct stat* sb) {
    struct Stat* s = head;
    while (s != NULL) {
        if (s->st_dev == sb->st_dev && s->st_ino == sb->st_ino) {
            return 1;
        }
        s = s->next;
    }
    return 0;
}

int main() {
    char* endptr;

    // parse TSTALL env var
    char* tstall_str = getenv("TSTALL");
    long tstall = 0;
    if (tstall_str != NULL) {
        tstall = strtol(tstall_str, &endptr, 10);
        if (errno != 0 || *endptr != '\0' || tstall <= 0)
            tstall = 0;
    }

    // parse UNITS env var
    char* units = getenv("UNITS");
    int in_kb = 0;
    if (units != NULL) {
        if (strcmp(units, "k") == 0 || strcmp(units, "K") == 0)
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
    struct Stat* stats;
    while (scanf("%s", filename) != EOF) {
        if (tstall)
            sleep(tstall);
        if (stat(filename, &sb) == -1)
            continue;
        if (!S_ISREG(sb.st_mode))
            continue;
        if (sb.st_nlink > 1) {
            if (contains(stats, &sb)) {
                continue;
            }
            insert(&stats, &sb);
        }
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

