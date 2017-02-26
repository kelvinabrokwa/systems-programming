/**
 * report.c
 * Author: Kelvin Abrokwa-Johnson
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define CLOSE_ALL_PIPES() close(stdin_to_accessed1_stdin[0]); \
    close(stdin_to_accessed1_stdin[1]); \
    close(stdin_to_accessed2_stdin[0]); \
    close(stdin_to_accessed2_stdin[1]); \
    close(accessed1_stdout_to_totalsize1_stdin[0]); \
    close(accessed1_stdout_to_totalsize1_stdin[1]); \
    close(accessed2_stdout_to_totalsize2_stdin[0]); \
    close(accessed2_stdout_to_totalsize2_stdin[1]); \
    close(totalsize1_stdout_to_report[0]); \
    close(totalsize1_stdout_to_report[1]); \
    close(totalsize2_stdout_to_report[0]); \
    close(totalsize2_stdout_to_report[1]) \

void print_usage() {
    fprintf(stderr, "This is report usage\n");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Not enough args\n");
        print_usage();
        exit(-1);
    }

    char* endptr;
    long num = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || num <= 0) {
        fprintf(stderr, "Invalid num argument %s\n", argv[1]);
        perror("strtol");
        print_usage();
        exit(-1);
    }

    // stdin to accessed1 stdin
    int stdin_to_accessed1_stdin[2];
    if (pipe(stdin_to_accessed1_stdin) == -1) {
        perror("pipe");
        exit(-1);
    }

    // accessed1 stdout to totalsize stdin
    int accessed1_stdout_to_totalsize1_stdin[2];
    if (pipe(accessed1_stdout_to_totalsize1_stdin) == -1) {
        perror("pipe");
        exit(-1);
    }

    // totalsize1 stdout to report
    int totalsize1_stdout_to_report[2];
    if (pipe(totalsize1_stdout_to_report) == -1) {
        perror("pipe");
        exit(-1);
    }

    // stdin to accessed2 stdin
    int stdin_to_accessed2_stdin[2];
    if (pipe(stdin_to_accessed2_stdin) == -1) {
        perror("pipe");
        exit(-1);
    }

    // accessed2 stdout to totalsize2 stdin
    int accessed2_stdout_to_totalsize2_stdin[2];
    if (pipe(accessed2_stdout_to_totalsize2_stdin) == -1) {
        perror("pipe");
        exit(-1);
    }

    // totalsize2 stdout to report
    int totalsize2_stdout_to_report[2];
    if (pipe(totalsize2_stdout_to_report) == -1) {
        perror("pipe");
        exit(-1);
    }


    /**
     * accessed1
     */
    int accessed1 = fork();
    if (accessed1 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(-1);
    }

    if (accessed1 == 0) {
        // connect report stdin to accessed1 stdin
        close(0);
        if (dup2(stdin_to_accessed1_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        // connect accessed1 stdout to totalsize1 stdin
        close(1);
        if (dup2(accessed1_stdout_to_totalsize1_stdin[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        CLOSE_ALL_PIPES();

        char num_arg[32];
        sprintf(num_arg, "%ld", num);
        char* args[] = {"accessed", num_arg, NULL};
        execv("accessed", args);

        perror("execv");
        fprintf(stderr, "Failed to exec accessed1\n");
        exit(-1);
    }

    /**
     * accessed2
     */
    int accessed2 = fork();
    if (accessed2 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(-1);
    }

    if (accessed2 == 0) {
        // connect report stdin to accessed1 stdin
        close(0);
        if (dup2(stdin_to_accessed2_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        // connect accessed1 stdout to totalsize1 stdin
        close(1);
        if (dup2(accessed2_stdout_to_totalsize2_stdin[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }
        
        CLOSE_ALL_PIPES();

        char num_arg[32];
        sprintf(num_arg, "-%ld", num);
        char* args[] = {"accessed", num_arg, NULL};
        execv("accessed", args);

        perror("execv");
        fprintf(stderr, "Failed to exec accessed2\n");
        exit(-1);
    }

    /**
     * totalsize1
     */
    int totalsize1 = fork();
    if (totalsize1 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(-1);
    }

    if (totalsize1 == 0) {
        // connect accessed1 stdout to totalsize1 stdin
        close(0);
        if (dup2(accessed1_stdout_to_totalsize1_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        // connect totalsize1 stdout to the pipe input
        close(1);
        if (dup2(totalsize1_stdout_to_report[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        CLOSE_ALL_PIPES();

        char* args[] = {NULL};
        char* env[] = {NULL};
        execve("totalsize", args, env);

        perror("execve");
        fprintf(stderr, "Failed to execve totalsize1\n");
        exit(-1);
    }

    /**
     * totalsize2
     */
    int totalsize2 = fork();
    if (totalsize2 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(-1);
    }

    if (totalsize2 == 0) {
        // connect accessed1 stdout to totalsize1 stdin
        close(0);
        if (dup2(accessed2_stdout_to_totalsize2_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        // connect totalsize1 stdout to the pipe input
        close(1);
        if (dup2(totalsize2_stdout_to_report[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(-1);
        }

        CLOSE_ALL_PIPES();

        char* args[] = {NULL};
        char* env[] = {NULL};
        execve("totalsize", args, env);

        perror("execve");
        fprintf(stderr, "Failed to execve totalsize2\n");
        exit(-1);
    }

    // report doesn't use these at all
    close(accessed1_stdout_to_totalsize1_stdin[0]);
    close(accessed1_stdout_to_totalsize1_stdin[1]);
    close(accessed2_stdout_to_totalsize2_stdin[0]);
    close(accessed2_stdout_to_totalsize2_stdin[1]);
    close(stdin_to_accessed1_stdin[0]);
    close(stdin_to_accessed2_stdin[0]);
    close(totalsize1_stdout_to_report[1]);
    close(totalsize2_stdout_to_report[1]);

    // write files from stdin to accessed1 and accessed2
    char filename[4096];
    while (scanf("%s", filename) != EOF) {
        write(stdin_to_accessed1_stdin[1], filename, strlen(filename));
        write(stdin_to_accessed1_stdin[1], " ", 1); // delimiter
        write(stdin_to_accessed2_stdin[1], filename, strlen(filename));
        write(stdin_to_accessed2_stdin[1], " ", 1); // delimiter
    }

    // report is done writing to these
    close(stdin_to_accessed1_stdin[1]);
    close(stdin_to_accessed2_stdin[1]);

    // read from pipes
    long val;
    read(totalsize1_stdout_to_report[0], &val, sizeof(int));
    printf("%ld\n", val);
    read(totalsize2_stdout_to_report[0], &val, sizeof(int));
    printf("%ld\n", val);

    return 0;
}

