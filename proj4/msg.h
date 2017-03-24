/**
 * msg.h
 */

#ifndef MSG_H
#define MSG_H

#define MAX_MSG_LEN      1024
#define MAX_HANDLE_LEN   1024

#define WHO              'a' // ask the client for a handle
#define OPP_HANDLE       'b' // tell the client the handle of its opponent
#define XO               'c' // tells client if they are x or o
#define MOVE             'd' // should contain board config
#define RESULT           'e' // should contain board config
#define MSG_DELIMITER    '\0'

// TODO: REMOVE ME
#define DB fprintf(stderr, "->\n")
#define DBN(n) fprintf(stderr, "%d\n", n)
#define DBC(c) fprintf(stderr, "%c\n", c)
#define DBS(s) fprintf(stderr, "%s\n", s)

struct Message {
    char* msg_str;
    int msg_int;
    char msg_char;
    char type;
};

struct MsgDgram {
    int nplayers; // number of player
    char player1[MAX_HANDLE_LEN]; // player 1 handle
    char player2[MAX_HANDLE_LEN]; // player 2 handle
};

int serialize_message(struct Message* msg);
int deserialize_message(struct Message* msg);
int write_message(int sockfd, struct Message* msg);
int read_message(int sockfd, struct Message* msg);
char symbol(int xo);
void print_board(char board[10]);

#endif
