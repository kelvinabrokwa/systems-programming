/**
 * Author: Kelvin Abrokwa-Johnson
 * rgpp.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>

/**
 * This data structure represents a file that has one 
 * or more matching lines
 * It also contains a next field for usage in a linked list
 */
struct File {
    char* name;
    struct Line* lines;
    struct File* next;
};

/**
 * This is essentially a node for a linked list of ints
 */
struct Line {
    int num;
    struct Line* next;
};

/**
 * insert at the end of the list
 * Returns 0 on success and 1 on error
 */
int insert_line_num(struct File* file, int line_num) 
{
    struct Line* line;
    line = (struct Line*)malloc(sizeof(struct Line)); 
    if (line == NULL) {
        fprintf(stderr, "Could not malloc\n");
        perror("malloc");
        return 1;
    }
    line->num = line_num;
    line->next = NULL;
    if (file->lines == NULL) {
        file->lines = line;
    } else {
        struct Line* curr = file->lines;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = line;
    }
    return 0;
}

/**
 * Insert at the head of list
 * Returns 0 on success and 1 on error
 */
int insert_result(struct File** head, char* file_name, int line_num)
{
    struct File* file = *head;

    while (file != NULL && strcmp(file->name, file_name) != 0) {
        file = file->next;
    }

    if (file == NULL) { // this is the first match in this file
        file = (struct File*)malloc(sizeof(struct File));
        if (file == NULL) {
            fprintf(stderr, "Could not malloc\n");
            perror("malloc");
            return 1;
        }
        char *fname = (char*)malloc(256);
        if (fname == NULL) {
            fprintf(stderr, "Could not malloc\n");
            perror("malloc");
            return 1;
        }
        strncpy(fname, file_name, 256);
        file->name = fname;
        file->next = *head;
        *head = file;
    }

    insert_line_num(file, line_num);

    return 0;
}

/**
 * Check if a linked list of lines contains a specified line number
 * Returns 1 if found and 0 if not found
 */
int contains(struct Line* lines, int line_num)
{
    struct Line* line = lines;
    while (line != NULL) {
        if (line->num == line_num)
            return 1;
        line = line->next;
    }
    return 0;
}

/**
 * Debugger method for printing the data structure
 */
void print_list(struct File* files)
{
    struct Line* line;
    struct File* file = files;
    while (file != NULL) {
        line = file->lines;
        while (line != NULL) {
            fprintf(stderr, "%s:%d\n", file->name, line->num);
            line = line->next;
        }
        file = file->next;
    }
}

/**
 * Prints usage information
 */
void print_usage() {
    char *usage = "\nUsage: rgpp [-l | -w word] {-n} {-b}\n"
                  "Processes output from grep\n"
                  "Must call grep with the following flags: -r -H -n -s -I -i\n"
                  "\nOptions:\n"
                  "  -l                     line mode\n"
                  "  -w word                word mode, highlights matching words\n"
                  "  -b                     show banner\n"
                  "  -n                     show line number\n";
    fprintf(stderr, "%s", usage);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }

    // Parse arguments
    int line_mode = 0, 
        word_mode = 0, 
        show_line_number = 0, 
        show_banner_line = 0;

    char* word;
    int word_len;

    int arg_idx = 1;

    if (strcmp(argv[1], "-l") == 0) {
        line_mode = 1;
        arg_idx++;
    } else if (strcmp(argv[1], "-w") == 0) {
        word_mode = 1;
        if (argc < 3) {
            fprintf(stderr, "\nError: The -w flag must have a word argument\n"); 
            print_usage();
            exit(1);
        }
        word = argv[2];
        word_len = strlen(word);
        arg_idx += 2;
    } else {
        fprintf(stderr, "\nError: First argument must be either -w or -l\n");
        print_usage();
        exit(1);
    }

    while (argc > arg_idx) {
        if (strcmp(argv[arg_idx], "-b") == 0) {
            show_banner_line = 1;
        } else if (strcmp(argv[arg_idx], "-n") == 0) {
            show_line_number = 1;
        } else if (strcmp(argv[arg_idx], "-nb") == 0 || strcmp(argv[arg_idx], "-bn") == 0) {
            show_banner_line = 1;
            show_line_number = 1;
        } else {
            fprintf(stderr, "\nError: Unrecognized flag \"%s\"\n", argv[arg_idx]);
            print_usage();
            exit(1);
        }
        arg_idx++;
    }

    // Read input from stdin into a linked list
    int matches = 0;
    int i;
    char line[2048];
    char* file_name;
    char* line_num_str;
    char* endptr;
    struct File* files = NULL;
    long ln;

    while (fgets(line, 1024, stdin) != NULL) {
        if (line_mode) {
            // count matching lines
            matches++;
        } else {
            // count matching words
            i = 0;
            while (line[i] != '\0') {
                if (strncasecmp(word, &line[i], word_len) == 0) {
                    i += word_len;
                    matches++;
                } else {
                    i++;
                }
            }
        }
        file_name = strtok(line, ":");
        line_num_str = strtok(NULL, ":");
        ln = strtol(line_num_str, &endptr, 10);
        if (errno != 0 || *endptr != '\0') {
            perror("strtol");
            print_usage();
            return 1;
        }
        insert_result(&files, file_name, (int)ln);
    }

    if (show_banner_line) {
        if (line_mode)
            printf("\nTHERE ARE %d LINES THAT MATCH\n\n", matches);
        else
            printf("\nTHERE ARE %d MATCHING WORDS\n\n", matches);
    }

    // Create rgpp output
    FILE* fp;
    int match;
    int line_num;
    int match_found;
    struct File* file;

    for (file = files;  file != NULL; file = file->next) {
        fp = fopen(file->name, "r");
        if (fp == NULL) {
            perror("fopen");
            return 1;
        }

        printf("=====================%s\n", file->name);

        line_num = 0;

        while (fgets(line, 2048, fp)) {
            line_num++;
            match_found = 0;
            match = contains(file->lines, line_num);

            if (line_mode) {
                if (match)
                    printf("*");
                else
                    printf(" ");
            }

            if (show_line_number)
                printf("%d: ", line_num);

            if (match) {
                i = 0;
                // search for matching substring
                while (line[i] != '\0') {
                    if (strncasecmp(word, &line[i], word_len) == 0) {
                        match_found = 1;
                        if (word_mode) {
                            printf("\e[7m"); // switch to inverse video
                            int j;
                            for (j = 0; j < word_len; j++) {
                                printf("%c", line[i]);
                                i++;
                            }
                            printf("\e[0m"); // switch back to normal
                        } else {
                            printf("%c", line[i]);
                            i++;
                        }
                    } else {
                        printf("%c", line[i]);
                        i++;
                    }
                }
            } else {
                printf("%s", line);
            }

            if (match && !match_found) {
                fprintf(stderr, "\nError: Could not find word \"%s\" in line %d\n"
                                "Possible mismatch between grepped word and "
                                "rgpp word argument\n", word, line_num);
                print_usage();
                exit(1);
            }
        }

        fclose(fp); // close file
    }

    return 0;
}

