/**
 * counter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define MAX_FILE_NAME_LEN 4095
#define MAX_LINE_LEN 2048
#define NSEC_IN_MSEC 1000 * 1000 // number of nanoseconds in a millisecond
#define MAX_WORD_LEN MAX_LINE_LEN

#define D() fprintf(stderr, ">>>>>\n")

struct Buffer {
    char **buffer;
    pthread_mutex_t lock;
    int readpos, writepos;
    pthread_cond_t notempty;
    pthread_cond_t notfull;
};

struct Word {
    char* word;
    int count;
    struct Word *next;
};

void init(struct Buffer* b);
void put(char *line);
char* get();
void* producer();
void* consumer();
long msec2nsec(int msec);
int insert_word(struct Word **list, char *w);
void print_word_list(struct Word* word);

struct Buffer buffer;

char **file_names;
char *end_cookie;
char *read_cookie;
int numfiles;
int numlines;
int numcounters;
int maxcounters;
int filedelay;
int threaddelay;
char threadname;
struct Word *even;
struct Word *odd;
pthread_t **counter_threads;
pthread_mutex_t even_lock;
pthread_mutex_t odd_lock;

/**
 *
 */
int insert_word(struct Word **list, char *w)
{
    // create the new node
    struct Word* word;
    word = (struct Word*)malloc(sizeof(struct Word));
    if (word == NULL) {
        fprintf(stderr, "Could not malloc word\n");
        perror("malloc");
        return 1;
    }
    word->word = (char*)malloc(MAX_WORD_LEN);
    word->count = 1;
    memcpy(word->word, w, strlen(w));
    word->next = NULL;

    // insert it into the list
    if (*list == NULL) {
        // the list is empty
        word->next = NULL;
        *list = word;
    }
    else if (strcmp((*list)->word, word->word) < 0) {
        // goes in front of the list
        word->next = *list;
        *list = word;
    }
    else if (strcmp((*list)->word, word->word) == 0) {
        // is the same as the front of the list
        free(word);
        (*list)->count++;
    }
    else {
        struct Word *curr = *list;
        while (curr->next != NULL && strcmp(curr->next->word, word->word) < 0)
            curr = curr->next;
        if (curr->next == NULL || strcmp(curr->next->word, word->word) != 0) {
            // place at appropriate spot
            word->next = curr->next;
            curr->next = word;
        } else {
            // already in list, increment counter
            curr->next->count++;
            free(word);
        }
    }

    return 0;
}

void print_word_list(struct Word* word)
{
    while (word != NULL) {
        printf("%s\t%d\n", word->word, word->count);
        word = word->next;
    }
}

/**
 *
 */
void init(struct Buffer* b)
{
    int i;
    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->notempty, NULL);
    pthread_cond_init(&b->notfull, NULL);
    b->readpos = 0;
    b->writepos = 0;
    b->buffer = (char**)calloc(numlines, sizeof(char*));
    for (i = 0; i < numlines; i++)
        b->buffer[i] = (char*)malloc(MAX_LINE_LEN);
}

/**
 *
 */
void put(char *line)
{
    // acquire the lock
    pthread_mutex_lock(&buffer.lock);

    // wait until buffer is not full
    while((buffer.writepos + 1) % numlines == buffer.readpos) {
        if (numcounters < maxcounters) {
            // create new counter thread
            pthread_t *counter_thread = (pthread_t*)malloc(sizeof(pthread_t));
            char *tname = (char*)malloc(1);
            *tname = threadname;
            pthread_create(counter_thread, NULL, consumer, (void*)tname);
            counter_threads[numcounters] = counter_thread;
            threadname++;
            numcounters++;
        }

        // wait on the not full condition
        pthread_cond_wait(&buffer.notfull, &buffer.lock);
    }

    // write data
    if (line != end_cookie) {
        memcpy(buffer.buffer[buffer.writepos], line, strlen(line));
    } else {
        free(buffer.buffer[buffer.writepos]); // prevent a memory leak
        buffer.buffer[buffer.writepos] = end_cookie;
    }

    // update write pointer
    buffer.writepos++;

    if (buffer.writepos >= numlines)
        buffer.writepos = 0;

    // signal the write
    pthread_cond_signal(&buffer.notempty);

    // release the lock
    pthread_mutex_unlock(&buffer.lock);
}

/**
 *
 */
void put_b1(char *line)
{
    // grab the lock
    pthread_mutex_lock(&buffer.lock);

    while (buffer.buffer[0] != read_cookie) {
        // there is unread data in the buffer
        pthread_cond_wait(&buffer.notfull, &buffer.lock);
    }

    // write data
    if (line != end_cookie) {
        buffer.buffer[0] = (char*)malloc(MAX_LINE_LEN);
        memcpy(buffer.buffer[0], line, strlen(line));
        //buffer.buffer[0] = line;
    } else {
        free(buffer.buffer[0]);
        buffer.buffer[0] = end_cookie;
    }

    // signal the write
    pthread_cond_signal(&buffer.notempty);

    // release the lock
    pthread_mutex_unlock(&buffer.lock);
}

/**
 * if NULL is returned then game over man
 */
char* get()
{
    char *line = (char*)malloc(MAX_LINE_LEN);

    // acquire the lock
    pthread_mutex_lock(&buffer.lock);

    // wait until buffer is not empty
    while (buffer.writepos == buffer.readpos) {
        pthread_cond_wait(&buffer.notempty, &buffer.lock);
    }

    // grab the data
    if (buffer.buffer[buffer.readpos] != end_cookie) {
        memcpy(line, buffer.buffer[buffer.readpos], strlen(buffer.buffer[buffer.readpos]));
    } else {
        // no more data
        // wake the next consumer up and return
        pthread_cond_signal(&buffer.notempty);
        // release the lock
        pthread_mutex_unlock(&buffer.lock);
        return NULL;
    }

    //update read pointer
    buffer.readpos++;
    if (buffer.readpos >= numlines)
        buffer.readpos = 0;

    // signal the read
    pthread_cond_signal(&buffer.notfull);

    // release the lock
    pthread_mutex_unlock(&buffer.lock);

    return line;
}

/**
 *
 */
char* get_b1()
{
    char *line = (char*)malloc(MAX_LINE_LEN);

    // acquire the lock
    pthread_mutex_lock(&buffer.lock);

    // wait for new data
    while (buffer.buffer[0] == read_cookie) {
        pthread_cond_wait(&buffer.notempty, &buffer.lock);
    }

    // grab the data
    if (buffer.buffer[0] != end_cookie) {
        memcpy(line, buffer.buffer[0], strlen(buffer.buffer[0]));
    } else {
        // no more data
        // wake the next consumer up and return
        pthread_cond_signal(&buffer.notempty);
        // release the lock
        pthread_mutex_unlock(&buffer.lock);
        return NULL;
    }

    free(buffer.buffer[0]);
    buffer.buffer[0] = read_cookie;

    // signal the read
    pthread_cond_signal(&buffer.notfull);

    // release the lock
    pthread_mutex_unlock(&buffer.lock);

    return line;
}

/**
 *
 */
void* producer()
{
    struct timespec req;
    FILE *file;
    int i;

    // TODO: lock?
    // this is so that the producer starts out being able to write
    if (numlines == 1) {
        buffer.buffer[0] = read_cookie;
    }

    // create the first consumer
    pthread_t *counter_thread = (pthread_t*)malloc(sizeof(pthread_t));
    char *tname = (char*)malloc(1);
    *tname = threadname;
    pthread_create(counter_thread, NULL, consumer, (void*)tname);
    counter_threads[numcounters] = counter_thread;
    threadname++;
    numcounters++;

    for (i = 0; i < numfiles; i++) {
        file = fopen(file_names[i], "r");
        if (file == NULL ||  errno != 0) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char line[MAX_LINE_LEN];

        while (fgets(line, sizeof(line), file)) {
            if (filedelay) {
                req.tv_sec = filedelay / 1000;
                req.tv_nsec = msec2nsec(filedelay - (filedelay / 1000));
                if (nanosleep(&req, NULL) != 0) {
                    fprintf(stderr, "%ld %ld\n", req.tv_sec, req.tv_nsec);
                    perror("nanosleep");
                }
            }
            if (numlines > 1)
                put(line);
            else
                put_b1(line);
        }
    }

    if (numlines > 1)
        put(end_cookie);
    else
        put_b1(end_cookie);

    fclose(file);

    for (i = 0; i < numcounters; i++) {
        if (counter_threads[i] != NULL)
            pthread_join(*(counter_threads[i]), NULL);
    }

    print_word_list(even);
    print_word_list(odd);

    return NULL;
}

/**
 *
 */
void* consumer(void *data)
{
    char name = *((char*)data);
    char *line, *word;
    struct timespec req;
    int wordlen;
    while (1) {
        if (numlines > 1)
            line = get();
        else
            line = get_b1();

        if (line == NULL)
            break;

        req.tv_sec = threaddelay / 1000;
        req.tv_nsec = msec2nsec(threaddelay - (threaddelay / 1000));
        if (nanosleep(&req, NULL) != 0) {
            fprintf(stderr, "%ld %ld\n", req.tv_sec, req.tv_nsec);
            perror("nanosleep");
        }

        word = strtok(line, " ");
        while (word != NULL) {
            wordlen = strlen(word);
            if (wordlen % 2) {
                pthread_mutex_lock(&odd_lock);
                insert_word(&odd, word);
                printf("%c", name);
                pthread_mutex_unlock(&odd_lock);
            } else {
                pthread_mutex_lock(&even_lock);
                insert_word(&even, word);
                printf("%c", name);
                pthread_mutex_unlock(&even_lock);
            }

            word = strtok(NULL, " ");
        }
    }
    return NULL;
}

/**
 *
 */
long msec2nsec(int msec)
{
    return (long)msec * NSEC_IN_MSEC;
}

int main(int argc, char** argv)
{
    if (argc < 10) {
        fprintf(stderr, "yo args aint enough\n");
        exit(EXIT_FAILURE);
    }

    numfiles = 0;
    file_names = (char**)calloc(argc - 9, sizeof(char*));
    numlines = -1;
    threaddelay = -1;
    filedelay = -1;
    maxcounters = -1;
    char* endptr;
    int i;
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-b", 2) == 0) {
            i++;
            if (i == argc) {
                fprintf(stderr, "-b flag needs an arg\n");
            }
            numlines = strtol(argv[i], &endptr, 10);
            if (errno != 0 || *endptr != '\0' || numlines <= 0) {
                fprintf(stderr, "-b flag needs a non-zero integer\n");
            }
        }
        else if (strncmp(argv[i], "-t", 2) == 0) {
            i++;
            if (i == argc) {
                fprintf(stderr, "-t flag needs an arg\n");
            }
            maxcounters = strtol(argv[i], &endptr, 10);
            if (errno != 0 || *endptr != '\0' || maxcounters <= 0 || maxcounters > 26) {
                fprintf(stderr, "0 < maxcounters <= 26\n");
            }
        }
        else if (strncmp(argv[i], "-d", 2) == 0) {
            i++;
            if (i == argc) {
                fprintf(stderr, "-d flag needs an arg\n");
            }
            filedelay = strtol(argv[i], &endptr, 10);
            if (errno != 0 || *endptr != '\0' || filedelay < 0) {
                fprintf(stderr, "0 < filedelay\n");
            }
        }
        else if (strncmp(argv[i], "-D", 2) == 0) {
            i++;
            if (i == argc) {
                fprintf(stderr, "-D flag needs an arg\n");
            }
            threaddelay = strtol(argv[i], &endptr, 10);
            if (errno != 0 || *endptr != '\0' || threaddelay < 0) {
                fprintf(stderr, "0 < threaddelay \n");
            }
        }
        else {
            file_names[numfiles] = argv[i];
            numfiles++;
        }
    }

    if (numfiles == 0) {
        fprintf(stderr, "You must pass file names\n");
        exit(EXIT_FAILURE);
    }
    if (numlines == -1) {
        fprintf(stderr, "You must pass a numlines flag\n");
        exit(EXIT_FAILURE);
    }
    if (filedelay == -1) {
        fprintf(stderr, "You must pass a filedelay flag\n");
        exit(EXIT_FAILURE);
    }
    if (threaddelay == -1) {
        fprintf(stderr, "You must pass a threaddelay flag\n");
        exit(EXIT_FAILURE);
    }

    end_cookie = (char*)malloc(1);
    read_cookie = (char*)malloc(1);
    pthread_t read_thread;
    even = NULL;
    odd = NULL;
    numcounters = 0;
    threadname = 'a';
    counter_threads = (pthread_t**)calloc(maxcounters, sizeof(pthread_t*));
    bzero(counter_threads, maxcounters);
    init(&buffer);
    pthread_create(&read_thread, NULL, producer, 0);
    pthread_join(read_thread, NULL);

    return 0;
}

