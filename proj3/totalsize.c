/**
 * totalsize.c
 * Author: Kelvin Abrokwa-Johnson
 */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    char filename[4096];
    long totalsize  = 0;
    struct stat sb;
    while (scanf("%s", filename) != EOF) {
        if (stat(filename, &sb) == -1)
            continue;
        totalsize += sb.st_size;
    }
    //printf("%ld\n", totalsize);
    write(1, &totalsize, 1);
    return 0;
}

