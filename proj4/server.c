/**
 * server.c
 */
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "msg.h"

//
// if player a is in STATE_REQ_MOVE or STATE_RES_MOVE
// player b should be in STATE_OPP_MOVE
//
#define STATE_WHO  1
#define STATE_WAIT 2
#define STATE_OPP_MOVE 4 // opponent is moving
#define STATE_REQ_MOVE 5 // should request a move
#define STATE_RES_MOVE 6 // waiting for a move request response

//
#define SEND_POSITIONS() \
    msg.type = XO; \
    msg.msg_char = 'x'; \
    write_message(xconn, &msg); \
    msg.type = XO; \
    msg.msg_char = 'o'; \
    write_message(oconn, &msg)

//
#define SEND_HANDLES() \
    msg.type = OPP_HANDLE; \
    msg.msg_str = xhandle; \
    write_message(oconn, &msg); \
    msg.type = OPP_HANDLE; \
    msg.msg_str = ohandle; \
    write_message(xconn, &msg)

//
#define SEND_RESULT(xo) \
    msg.type = RESULT; \
    msg.msg_char = xo; \
    msg.msg_str = board; \
    write_message(xconn, &msg); \
    write_message(oconn, &msg); \
    close(xconn); \
    close(oconn); \
    xconn = -1; \
    oconn = -1

/**
 * Debugger method
 */
void printsin(struct sockaddr_in *sin, char *m1, char *m2 )
{
    char fromip[INET_ADDRSTRLEN];

    printf ("%s %s:\n", m1, m2);
    printf ("  family %d, addr %s, port %d\n",
            sin->sin_family,
	        inet_ntop(AF_INET, &(sin->sin_addr.s_addr), fromip, sizeof(fromip)),
            ntohs((unsigned short)(sin -> sin_port)));
}

/**
 * returns 1 if the player has won a game and 0 if they haven't
 */
int has_won(char board[10], char xo)
{
    int i;

    // every way to win
    int wins[8][3] = {
        {0, 1, 2},
        {3, 4, 5},
        {6, 7, 8},
        {0, 3, 6},
        {1, 4, 7},
        {2, 5, 8},
        {0, 4, 8},
        {2, 4, 6}
    };

    for (i = 0; i < 8; i++) {
        if (board[wins[i][0]] == xo &&
            board[wins[i][1]] == xo &&
            board[wins[i][2]] == xo) {
            return 1;
        }
    }

    return 0;
}

void reset_board(char board[10])
{
    int i;
    for (i = 0; i < 9; i++)
        board[i] = ' ';
    board[9] = '\0';
}


int main()
{
    int listener; // fd for socket upon which we get connection requests
    int xconn; // fd for x socket
    int oconn; // fd for o socket
    int sock_dgram_fd; // datagram socket
    int xstate;
    int ostate;
    int ecode;
    int nbytes;
    struct sockaddr_in *localaddr, oaddr, xaddr, dgram_sin;
    struct addrinfo hints, *addrlist;
    socklen_t addrlen;
    char board[10];
    char msg_buf[MAX_MSG_LEN];
    char *endptr;
    struct Message msg;
    char xhandle[MAX_HANDLE_LEN], ohandle[MAX_HANDLE_LEN];
    struct MsgDgram msg_dgram;

    //
    // Set up stream socket listener
    //
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    ecode = getaddrinfo(NULL, "0", &hints, &addrlist);
    if (ecode != 0) {
        fprintf(stderr, "SERVER::ERROR:: getaddrinfo: %s\n", gai_strerror(ecode));
        exit(EXIT_FAILURE);
    }
    localaddr = (struct sockaddr_in*)addrlist->ai_addr;
    listener = socket(addrlist->ai_family, addrlist->ai_socktype, 0);
    if (listener < 0) {
        perror("server:socket");
        exit(EXIT_FAILURE);
    }
    if (bind(listener, (struct sockaddr*)localaddr, sizeof(struct sockaddr_in)) < 0) {
        perror("SERVER::ERROR:: bind");
        exit(EXIT_FAILURE);
    }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(listener, (struct sockaddr*)localaddr, &addrlen) < 0) {
        perror("SERVER::ERROR:: getsockname");
        exit(EXIT_FAILURE);
    }
    listen(listener, 4);

    // write server machine and port to file
    char ip[INET_ADDRSTRLEN];
    FILE *f = fopen("server_addr.txt", "w");
    if (f == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(f, "%s\n", inet_ntop(AF_INET, &(localaddr->sin_addr.s_addr), ip, sizeof(ip)));
    fprintf(f, "%d\n", ntohs((unsigned short)localaddr->sin_port));

#ifdef DEBUG
    fprintf(stderr, "SERVER::DEBUG:: stream server listening on port: %d\n",
            ntohs((unsigned short)localaddr->sin_port));
#endif /* DEBUG */

    //
    // Set up datagram socket listener
    //
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    ecode = getaddrinfo(NULL, "13107", &hints, &addrlist);
    if (ecode != 0) {
        fprintf(stderr, "SERVER::ERROR:: getaddrinfo: %s\n", gai_strerror(ecode));
        exit(EXIT_FAILURE);
    }
    localaddr = (struct sockaddr_in*)addrlist->ai_addr;
#ifdef DEBUG
    printsin(localaddr, "SERVER::DEBUG:: RECV_UDP", "Local socket is");
#endif /* DEBUG */
    sock_dgram_fd = socket(addrlist->ai_family, addrlist->ai_socktype, 0);
    if (sock_dgram_fd < 0) {
        perror("SERVER::ERROR:: Could not create UDP socket");
        exit(EXIT_FAILURE);
    }
    if (bind(sock_dgram_fd, (struct sockaddr*)localaddr, sizeof(struct sockaddr_in))) {
        perror("SERVER::ERROR:: bind");
        exit(EXIT_FAILURE);
    }

    // write datagram port to file
    fprintf(f, "%d", ntohs((unsigned short)localaddr->sin_port));
    fclose(f);

    // when these file descriptors are -1
    // no one is connected on these sockets
    xconn = -1;
    oconn = -1;

    fd_set rfds;
    int i;
    char c;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(listener, &rfds);
        FD_SET(sock_dgram_fd, &rfds);
        if (xconn != -1)
            FD_SET(xconn, &rfds);
        if (oconn != -1)
            FD_SET(oconn, &rfds);

        if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i <= FD_SETSIZE; i++) {
            if (FD_ISSET(i, &rfds)) {
                // check if a new client is trying to connect
                if (i == sock_dgram_fd) {
                    addrlen = sizeof(dgram_sin);
                    recvfrom(sock_dgram_fd, &msg_dgram, sizeof(msg_dgram), 0,
                            (struct sockaddr*)&dgram_sin, &addrlen);
#ifdef DEBUG
                    printsin(&dgram_sin, "SERVER::DEBUG::", "Packet from");
#endif /* DEBUG */
                    msg_dgram.nplayers = 0;
                    if (xconn != -1) {
                        msg_dgram.nplayers++;
                        strncpy(msg_dgram.player1, xhandle, MAX_HANDLE_LEN);
                    }
                    if (oconn != -1) {
                        msg_dgram.nplayers++;
                        strncpy(msg_dgram.nplayers == 1 ?
                                msg_dgram.player1 : msg_dgram.player2, ohandle, MAX_HANDLE_LEN);
                    }
                    sendto(sock_dgram_fd, &msg_dgram, sizeof(msg_dgram), 0,
                            (struct sockaddr*)&dgram_sin, addrlen);
                } else if (i == listener) {
                    if (xconn == -1) {
                        xconn = accept(listener, (struct sockaddr*)&xaddr, &addrlen);
                        if (xconn < 0) {
                            perror("server:accept");
                            exit(EXIT_FAILURE);
                        }
#ifdef DEBUG
                        printsin(&xaddr, "SERVER::DEBUG::", "accepted x connection from");
#endif /* DEBUG */

                        // write who
                        msg.type = WHO;
                        msg.msg_str = NULL;
                        write_message(xconn, &msg);

                        // set state of x player
                        xstate = STATE_WHO;
                    }
                    else if (oconn == -1) {
                        oconn = accept(listener, (struct sockaddr*)&oaddr, &addrlen);
                        if (oconn < 0) {
                            perror("server:accept");
                            exit(EXIT_FAILURE);
                        }
#ifdef DEBUG
                        printsin(&oaddr,"SERVER::DEBUG::", "accepted o connection from");
#endif /* DEBUG */

                        // write who
                        msg.type = WHO;
                        msg.msg_str = NULL;
                        write_message(oconn, &msg);

                        // set state of o player
                        ostate = STATE_WHO;
                    }
                }

                // read message from x
                if (i == xconn) {
                    msg.msg_str = msg_buf; // make sure msg.msg_str has enough space to
                                           // hold any message string
                    ecode = read_message(xconn, &msg);
                    if (ecode == -1) {
                        xconn = -1;
#ifdef DEBUG
                        fprintf(stderr, "SERVER::DEBUG:: x client disconnected\n");
#endif /* DEBUG */
                        break;
                    }

                    // TODO: check for other read_message errors

                    switch (msg.type) {
                        case WHO:
                            if (xstate != STATE_WHO) {
                                // bail
                            }

                            strncpy(xhandle, msg.msg_str, MAX_HANDLE_LEN);
#ifdef DEBUG
                            fprintf(stderr, "SERVER::DEBUG:: x handle: %s\n", xhandle);
#endif /* DEBUG */

                            if (ostate == STATE_WAIT) {
                                // start the game
#ifdef DEBUG
                                fprintf(stderr, "SERVER::DEBUG:: starting game\n");
#endif /* DEBUG */
                                reset_board(board);

                                SEND_HANDLES();
                                SEND_POSITIONS();

                                // adjust state
                                ostate = STATE_OPP_MOVE;
                                xstate = STATE_REQ_MOVE;
                            } else {
                                xstate = STATE_WAIT;
                            }

                            break;

                        case MOVE:
                            if (xstate != STATE_RES_MOVE) {
                                fprintf(stderr, "SERVER::ERROR:: x should be in STATE_RES_MOVE. "
                                                "It is in %d\n", xstate);
                                exit(EXIT_FAILURE);
                            }
#ifdef DEBUG
                            fprintf(stderr, "SERVER::DEBUG:: Handling move from x: %d\n", msg.msg_int);
#endif /* DEBUG */

                            assert(board[msg.msg_int] == ' ');
                            board[msg.msg_int] = 'x';

                            if (has_won(board, 'x')) {
                                // tell both that x won
#ifdef DEBUG
                                fprintf(stderr, "SERVER::DEBUG:: x wins\n");
#endif
                                SEND_RESULT('x');
                            } else {
                                // adjust state
                                ostate = STATE_REQ_MOVE;
                                xstate = STATE_OPP_MOVE;
                            }

                            break;

                        default:
                            fprintf(stderr, "SERVER::ERROR:: Invalid message type from x player\n");
                            exit(EXIT_FAILURE);
                    }
                }

                // read message from o
                if (i == oconn) {
                    msg.msg_str = msg_buf; // make sure msg.msg_str has enough space to
                                           // hold any message string
                    ecode = read_message(oconn, &msg);
                    if (ecode == -1) {
                        oconn = -1;
#ifdef DEBUG
                        fprintf(stderr, "SERVER::DEBUG:: o client disconnected\n");
#endif /* DEBUG */
                        break;
                    }

                    // TODO: check for other read_message errors

                    switch (msg.type) {
                        case WHO:
                            if (ostate != STATE_WHO) {
                                // bail
                            }

                            strncpy(ohandle, msg.msg_str, MAX_HANDLE_LEN);
#ifdef DEBUG
                            fprintf(stderr, "SERVER::DEBUG:: o handle %s\n", xhandle);
#endif /* DEBUG */

                            if (xstate == STATE_WAIT) {
                                // start the game
#ifdef DEBUG
                                fprintf(stderr, "SERVER::DEBUG:: starting game\n");
#endif /* DEBUG */
                                reset_board(board);

                                SEND_HANDLES();
                                SEND_POSITIONS();

                                // adjust state
                                xstate = STATE_OPP_MOVE;
                                ostate = STATE_REQ_MOVE;
                            } else {
                                ostate = STATE_WAIT;
                            }

                            break;

                        case MOVE:
                            if (ostate != STATE_RES_MOVE) {
                                fprintf(stderr, "SERVER::ERROR:: o should be in STATE_RES_MOVE. "
                                                "It is in %d\n", ostate);
                                exit(EXIT_FAILURE);
                            }
#ifdef DEBUG
                            fprintf(stderr, "SERVER::DEBUG:: Handling move from o: %d\n", msg.msg_int);
#endif /* DEBUG */

                            assert(board[msg.msg_int] == ' ');
                            board[msg.msg_int] = 'o';

                            if (has_won(board, 'o')) {
                                // tell both that x won
#ifdef DEBUG
                                fprintf(stderr, "SERVER::DEBUG:: o wins\n");
#endif
                                SEND_RESULT('o');
                            } else {
                                // adjust state
                                xstate = STATE_REQ_MOVE;
                                ostate = STATE_OPP_MOVE;
                            }

                            break;

                        default:
                            fprintf(stderr, "SERVER::DEBUG:: Invalid message type from o player\n");
                            exit(EXIT_FAILURE);
                    }
                }
            }

            if (xstate == STATE_REQ_MOVE) {
#ifdef DEBUG
                fprintf(stderr, "SERVER::DEBUG:: resquesting move from x\n");
#endif /* DEBUG */
                // request move from x
                msg.type = MOVE;
                msg.msg_str = board;
                write_message(xconn, &msg);

                // update state
                ostate = STATE_OPP_MOVE; // this may be redundant
                xstate = STATE_RES_MOVE;
            } else if (ostate == STATE_REQ_MOVE) {
#ifdef DEBUG
                fprintf(stderr, "SERVER::DEBUG:: resquesting move from o\n");
#endif /* DEBUG */
                // request move from o
                msg.type = MOVE;
                msg.msg_str = board;
                write_message(oconn, &msg);

                // update state
                xstate = STATE_OPP_MOVE; // this may be redundant
                ostate = STATE_RES_MOVE;

            }
        }
    }
}

