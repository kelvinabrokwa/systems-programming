/**
 * Author: Kelvin Abrokwa-Johnson
 * Date: 25 January 2017
 * Course: Systems Programming
 * Instructor: Professor Kearns
 * Compiling: gcc -Wall -o beetle beetle.c -lm
 *
 * This program is a Monte Carlo simulation of the
 * average life span of an intoxicated beetle on a
 * board hanging over a cauldron of boiling oil
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#define PI (3.141592653589793)

int main(int argc, char** argv) {
    char* usage = "Usage: ./beetle <sheet size> <repetitions>\n"
        "sheet size repetitions must be postitive nonzero integers\n";
    if (argc != 3) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }
    errno = 0;
    char *endptr;
    long size = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || size <= 0) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }
    errno = 0;
    long reps = strtol(argv[2], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || reps <= 0) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }
    long time = 0, rep;
    float x, y, angle, center = size / 2.;
    for (rep = 0; rep < reps; rep++) {
        x = y = center;
        while (x > 0 && x < size && y > 0 && y < size) {
            time += 2; // walk and sleep
            angle = (random() / (float)RAND_MAX) * 2 * PI;
            x += cos(angle);
            y += sin(angle);
        }
        time--; // discount last sleep
    }
    double lifetime_average = time / (double)reps;
    printf("%ld by %ld square, %ld beetles, mean beetle lifetime is %.1lf\n", 
            size, size, reps, lifetime_average);
    return 0;
}
