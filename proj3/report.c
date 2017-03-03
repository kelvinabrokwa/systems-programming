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
#include <sys/types.h>
#include <sys/wait.h>

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

int signal_flag;

void signal_handler() {
    signal_flag = 1;
}

void print_usage() {
    fprintf(stderr, "\nUsage: ls | report <number of days> {-d <delay>} {-k}\n"
                    "Creates a report of how recently files were modified\n"
                    "\nOptions\n"
                    "  -d num     number of seconds to delay between inputs\n"
                    "  -k         print file size in kilobytes\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Not enough args\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    // handle signals from totalsize
    signal_flag = 0;
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
        if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "-K") == 0) {
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
    int num_env_vars = 2; // always required TMOM variable and NULL terminator

    if (in_kb)
        num_env_vars++;

    if (delay)
        num_env_vars++;

    char* env[num_env_vars];

    int env_idx = 0;

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
    close(stdin_to_accessed1_stdin[0]);
    close(stdin_to_accessed2_stdin[0]);
    close(accessed1_stdout_to_totalsize1_stdin[0]);
    close(accessed1_stdout_to_totalsize1_stdin[1]);
    close(accessed2_stdout_to_totalsize2_stdin[0]);
    close(accessed2_stdout_to_totalsize2_stdin[1]);
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


    while (!signal_flag) {
        sleep(1);
        printf("*");
        fflush(stdout);
    }

    printf("\n");

    // read from pipes
    char val[64];
    char c;

    // These two very ugly reads do it byte by byte so that we stop
    // at the newline character

    // read from totalsize1
    int idx = 0;
    if (read(totalsize1_stdout_to_report[0], &c, 1) == -1) {
        fprintf(stderr, "ERROR: Failed to read from totalsize1 output pipe\n");
        perror("read");
        exit(EXIT_FAILURE);
    }
    while (c != '\n') {
        val[idx] = c;
        idx++;
        if (read(totalsize1_stdout_to_report[0], &c, 1) == -1) {
            fprintf(stderr, "ERROR: Failed to read from totalsize1 output pipe\n");
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
    val[idx] = '\0'; // terminate string

    printf("A total of %s%s are in regular files not accessed for 1 days.\n",
            val, in_kb ? "" : " bytes");

    printf("----------\n");

    // read from totalsize2
    idx = 0;
    if (read(totalsize2_stdout_to_report[0], &c, 1) == -1) {
        fprintf(stderr, "ERROR: Failed to read from totalsize1 output pipe\n");
        perror("read");
        exit(EXIT_FAILURE);
    }
    while (c != '\n') {
        val[idx] = c;
        idx++;
        if (read(totalsize2_stdout_to_report[0], &c, 1) == -1) {
            fprintf(stderr, "ERROR: Failed to read from totalsize1 output pipe\n");
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
    printf("A total of %s%s are in regular files accessed within 1 days.\n",
            val, in_kb ? "" : " bytes");

    // close remaining pipes for good measure
    close(totalsize1_stdout_to_report[0]);
    close(totalsize2_stdout_to_report[0]);

    // wait for kids
    int status;
    waitpid(accessed1, &status, 0);
    waitpid(accessed2, &status, 0);
    waitpid(totalsize1, &status, 0);
    waitpid(totalsize2, &status, 0);

    return 0;
}

