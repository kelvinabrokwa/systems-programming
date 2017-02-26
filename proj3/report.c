/**
 * report.c
 * Author: Kelvin Abrokwa-Johnson
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

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

int totalsize1_stdout_to_report_read;
int totalsize2_stdout_to_report_read;

void signal_handler();

void print_usage() {
    fprintf(stderr, "This is report usage\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough args\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    // handle signals from totalsize
    signal(SIGUSR1, signal_handler);

    char* endptr;

    // parse num argument
    long num = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || num <= 0) {
        fprintf(stderr, "Invalid num argument %s\n", argv[1]);
        perror("strtol");
        print_usage();
        exit(EXIT_FAILURE);
    }

    // parse delay and kilobyte arguments
    int in_kb = 0;
    long delay = 0;
    int i;
    for (i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-k", 2) == 0) {
            in_kb = 1;
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            if (argc <= i + 1) {
                fprintf(stderr, "-d flag must be followed by a non-zero integer\n");
                print_usage();
                exit(EXIT_FAILURE);
            }
            delay = strtol(argv[i + 1], &endptr, 10);
            if (errno != 0 || *endptr != '\0' || delay <= 0) {
                fprintf(stderr, "-d argument must be a non-zero integer. "
                                "You entered %s\n", argv[i + 1]);
                perror("strtol");
                print_usage();
                exit(EXIT_FAILURE);
            }
        }
    }

    // build env array for totalsize
    int num_env_vars = 1; // always required TMOM variable
    int env_idx = 0;

    if (in_kb)
        num_env_vars++;

    if (delay)
        num_env_vars++;

    char* env[num_env_vars];

    char tmom[128];
    sprintf(tmom, "TMOM=%d", (int)getpid());
    env[env_idx] = tmom;
    env_idx++;

    if (in_kb) {
        env[env_idx] = "UNITS=k";
        env_idx++;
    }

    if (delay) {
        char tstall[128];
        sprintf(tstall, "TSTALL=%ld", delay);
        env[env_idx] = tstall;
        env_idx++;
    }

    env[env_idx] = NULL; // null terminate the array

    fprintf(stderr, "%s\n", env[0]);
    fprintf(stderr, "%s\n", env[1]);
    fprintf(stderr, "%s\n", env[2]);
    fprintf(stderr, "%s\n", env[3]);

    /**
     * Create pipes
     */

    // stdin to accessed1 stdin
    int stdin_to_accessed1_stdin[2];
    if (pipe(stdin_to_accessed1_stdin) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // accessed1 stdout to totalsize stdin
    int accessed1_stdout_to_totalsize1_stdin[2];
    if (pipe(accessed1_stdout_to_totalsize1_stdin) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // totalsize1 stdout to report
    int totalsize1_stdout_to_report[2];
    if (pipe(totalsize1_stdout_to_report) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // stdin to accessed2 stdin
    int stdin_to_accessed2_stdin[2];
    if (pipe(stdin_to_accessed2_stdin) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // accessed2 stdout to totalsize2 stdin
    int accessed2_stdout_to_totalsize2_stdin[2];
    if (pipe(accessed2_stdout_to_totalsize2_stdin) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // totalsize2 stdout to report
    int totalsize2_stdout_to_report[2];
    if (pipe(totalsize2_stdout_to_report) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // get these file descriptors global for the sake of the interrupt handler
    totalsize1_stdout_to_report_read = totalsize1_stdout_to_report[0];
    totalsize2_stdout_to_report_read = totalsize2_stdout_to_report[0];

    /**
     * accessed1
     */
    int accessed1 = fork();
    if (accessed1 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(EXIT_FAILURE);
    }

    if (accessed1 == 0) {
        // connect report stdin to accessed1 stdin
        close(0);
        if (dup2(stdin_to_accessed1_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // connect accessed1 stdout to totalsize1 stdin
        close(1);
        if (dup2(accessed1_stdout_to_totalsize1_stdin[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        CLOSE_ALL_PIPES();

        char num_arg[32];
        sprintf(num_arg, "%ld", num);
        char* args[] = {"accessed", num_arg, NULL};
        execv("accessed", args);

        perror("execv");
        fprintf(stderr, "Failed to exec accessed1\n");
        exit(EXIT_FAILURE);
    }

    /**
     * accessed2
     */
    int accessed2 = fork();
    if (accessed2 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(EXIT_FAILURE);
    }

    if (accessed2 == 0) {
        // connect report stdin to accessed1 stdin
        close(0);
        if (dup2(stdin_to_accessed2_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // connect accessed1 stdout to totalsize1 stdin
        close(1);
        if (dup2(accessed2_stdout_to_totalsize2_stdin[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        CLOSE_ALL_PIPES();

        char num_arg[32];
        sprintf(num_arg, "-%ld", num);
        char* args[] = {"accessed", num_arg, NULL};
        execv("accessed", args);

        perror("execv");
        fprintf(stderr, "Failed to exec accessed2\n");
        exit(EXIT_FAILURE);
    }
    
    /**
     * totalsize1
     */
    int totalsize1 = fork();
    if (totalsize1 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(EXIT_FAILURE);
    }
    
    if (totalsize1 == 0) {
        // connect accessed1 stdout to totalsize1 stdin
        close(0);
        if (dup2(accessed1_stdout_to_totalsize1_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // connect totalsize1 stdout to the pipe input
        close(1);
        if (dup2(totalsize1_stdout_to_report[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        CLOSE_ALL_PIPES();

        char* args[] = {NULL};
        execve("totalsize", args, env);

        perror("execve");
        fprintf(stderr, "Failed to execve totalsize1\n");
        exit(EXIT_FAILURE);
    }

    /**
     * totalsize2
     */
    int totalsize2 = fork();
    if (totalsize2 == -1) {
        fprintf(stderr, "failed fork\n");
        exit(EXIT_FAILURE);
    }

    if (totalsize2 == 0) {
        // connect accessed1 stdout to totalsize1 stdin
        close(0);
        if (dup2(accessed2_stdout_to_totalsize2_stdin[0], 0) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // connect totalsize1 stdout to the pipe input
        close(1);
        if (dup2(totalsize2_stdout_to_report[1], 1) == -1) {
            fprintf(stderr, "Could not dup\n");
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        CLOSE_ALL_PIPES();

        char* args[] = {NULL};
        execve("totalsize", args, env);

        perror("execve");
        fprintf(stderr, "Failed to execve totalsize2\n");
        exit(EXIT_FAILURE);
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

    while (1) {
        sleep(1);
        printf("*");
        fflush(stdout);
    }

    return 0;
}

void signal_handler() {
    // read from pipes
    char val[64];

    // read from totalsize1
    read(totalsize1_stdout_to_report_read, &val, sizeof(val));
    printf("%s\n", val);

    // read from totalsize2
    read(totalsize2_stdout_to_report_read, &val, sizeof(val));
    printf("%s\n", val);
}
