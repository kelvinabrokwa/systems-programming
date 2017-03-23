/**
 * msg.c
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "msg.h"

/**
 *
 */
int write_message(int sockfd, struct Message* msg)
{
    char buf[MAX_MSG_LEN];
    int len;

    switch (msg->type) {
        case WHO:
            if (msg->msg_str == NULL) {
                sprintf(buf, "%c%c", WHO, MSG_DELIMITER);
            } else {
                sprintf(buf, "%c%s%c", WHO, msg->msg_str, MSG_DELIMITER);
            }
            break;

        case MOVE:
            if (msg->msg_str != NULL) {
                sprintf(buf, "%c%s%c", MOVE, msg->msg_str, MSG_DELIMITER);
            } else {
                sprintf(buf, "%c%d%c", MOVE, msg->msg_int, MSG_DELIMITER);
            }
            break;

        case OPP_HANDLE:
            sprintf(buf, "%c%s%c", OPP_HANDLE, msg->msg_str, MSG_DELIMITER);
            break;

        case XO:
            sprintf(buf, "%c%c%c", XO, msg->msg_char, MSG_DELIMITER);
            break;

        case RESULT:
            sprintf(buf, "%c%s%c", RESULT, msg->msg_str, MSG_DELIMITER);
            break;
    }

#ifdef DEBUG
    fprintf(stderr, "WRITE MESSAGE::%s\n", buf);
#endif

    // send it
    write(sockfd, buf, strlen(buf) + 1);

    return 0;
}

/**
 * returns the message type read and stores info into a buffer
 * returns -1 if the client has disconnected
 * returns -2 if the message is fudged up
 */
int read_message(int sockfd, struct Message* msg)
{
    char buf[MAX_MSG_LEN];
    int i = 0;
    char c, *p, type;
    int nbytes;
    char* endptr;

    // TODO: disconnect handling

    do {
        if ((nbytes = read(sockfd, &c, 1)) != 1) {
            if (nbytes == 0) {
                return -1;
            } else {
                fprintf(stderr, "MSG:: read_message:: Could not read byte. Read %d\n", nbytes);
                exit(EXIT_FAILURE);
            }
        }
        buf[i] = c;
        i++;
    } while (c != MSG_DELIMITER);

    type = buf[0];
    char* b = buf + 1;

    switch (type) {
        case WHO:
            msg->type = WHO;
            if (*b == MSG_DELIMITER) {
                // read a request
                msg->msg_str = NULL;
            } else {
                // read a response
                msg->msg_str = (char *)malloc(MAX_HANDLE_LEN);
                p = msg->msg_str;
                while ((c = *b) != MSG_DELIMITER) {
                    *p = c;
                    p++;
                    b++;
                }
            }
            break;

        case MOVE:
            msg->type = MOVE;
            if (strlen(b) == 1) {
                // read a move
                msg->msg_int = (int)strtol(b, &endptr, 10);
                if (errno != 0) {
                    perror("strtol");
                    fprintf(stderr, "MSG:: Could not read move. Sent: %s", b);
                    return -2;
                }
                if (msg->msg_int < 0 || msg->msg_int > 9) {
                    fprintf(stderr, "Invalid move position. Sent: %s", b);
                    return -2;
                }
            } else if (strlen(b) == 9) {
                // read a move request
                // contains the board
                // FIXME
                msg->msg_str = b;
            } else {
                fprintf(stderr, "MSG::READ::Invalid MOVE message length: %lu %s\n", strlen(b), b);
                return -2;
            }
            break;

        case OPP_HANDLE:
            msg->type = OPP_HANDLE;
            strncpy(msg->msg_str, b, MAX_HANDLE_LEN);
            break;

        case XO:
            msg->type = XO;
            if (strlen(b) != 1) {
                fprintf(stderr, "MSG::READ::Invalid XO message length: %lu\n", strlen(b));
                return -2;
            }
            msg->msg_char = *b;
            break;

        case RESULT:
            msg->type = RESULT;
            if (strlen(b) != 9) {
                fprintf(stderr, "MSG::READ::Invalid RESULT message length: %lu\n", strlen(b));
                return -2;
            }
            strncpy(msg->msg_str, b, strlen(b));
            break;

        default:
            fprintf(stderr, "MSG::READ::Invalide message type %d\n", type);
    }

    return 0;
}

/**
 *
 */
void print_board(char board[10])
{
    printf("%c | %c | %c\n"
           "------------\n"
           "%c | %c | %c\n"
           "------------\n"
           "%c | %c | %c\n",
           board[0], board[1], board[2],
           board[3], board[4], board[5],
           board[6], board[7], board[8]);
}

