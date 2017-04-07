/**
 *
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
#define DC(cond) if (cond) { D(); }

struct Buffer {
    char **buffer;
    pthread_mutex_t lock;
    int readpos, writepos;
    pthread_cond_t notempty;
    pthread_cond_t notfull;
};

struct Word {
    char* word;
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
int numlines;
int numcounters;
int maxcounters;
int threaddelay;
char threadname;
struct Word *even;
struct Word *odd;
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
    memcpy(word->word, w, strlen(w));
    word->next = NULL;

    // insert it into the list
    if (*list == NULL || strcmp((*list)->word, word->word) < 0) {
        // the list is empty
        word->next = *list == NULL ? NULL : (*list)->next;
        *list = word;
    } else {
        struct Word *curr = *list;
        while (curr->next != NULL && strcmp(curr->next->word, word->word) < 0)
            curr = curr->next;
        word->next = curr->next;
        curr->next = word;
    }

    return 0;
}

void print_word_list(struct Word* word)
{
    while (word != NULL) {
        printf("%s\n", word->word);
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
            pthread_t counter_thread;
            char *tname = (char*)malloc(1);
            *tname = threadname;
            pthread_create(&counter_thread, NULL, consumer, (void*)tname);
            threadname++;
            numcounters++;
        }

        // wait on the not full condition
        pthread_cond_wait(&buffer.notfull, &buffer.lock);
    }

    // write data
    memcpy(buffer.buffer[buffer.writepos], line, strlen(line));

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
    memcpy(line, buffer.buffer[buffer.readpos], strlen(buffer.buffer[buffer.readpos]));

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
void* producer()
{
    char filename[] = "/usr/share/fortune/linuxcookie";

    FILE *file = fopen(filename, "r");
    if (file == NULL ||  errno != 0) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), file)) {
        put(line);
    }

    fclose(file);

    // TODO: remember to join on all created consumer threads
    print_word_list(even);

    return NULL;
}

/**
 *
 */
void* consumer(void *data)
{
    //char name = *((char*)data);
    char *line, *word;
    struct timespec req;
    int wordlen;
    while (1) {
        line = get();
        req.tv_sec = 0;
        req.tv_nsec = msec2nsec(threaddelay);
        if (nanosleep(&req, NULL) != 0) {
            perror("nanosleep");
        }
        word = strtok(line, " ");
        while (word != NULL) {
            wordlen = strlen(word);
            if (wordlen % 2) {
                pthread_mutex_lock(&odd_lock);
                insert_word(&odd, word);
                pthread_mutex_unlock(&odd_lock);
            } else {
                pthread_mutex_lock(&even_lock);
                insert_word(&even, word);
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

int main()
{
    pthread_t read_thread;
    void *retval;
    even = NULL;
    odd = NULL;
    numlines = 5;
    numcounters = 0;
    maxcounters = 10;
    threaddelay = 100;
    threadname = 'a';
    init(&buffer);
    pthread_create(&read_thread, NULL, producer, 0);
    pthread_join(read_thread, &retval);
    return 0;
}

