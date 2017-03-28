/**
 * client.c
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#include "msg.h"

void print_usage()
{
    fprintf(stderr, "Usage: ./client {-q | -t timeout}\n");
}

/**
 * returns -1 if the file does not exist
 */
int get_server_info(char *server_ip, char *server_stream_port, char *server_dgram_port)
{
    // read server information from file
    FILE *f = fopen("server_addr.txt", "r");
    if (f == NULL) {
        return -1;
    }
    if (fgets(server_ip, INET_ADDRSTRLEN, f) == NULL) {
        perror("CLIENT::ERROR:: fgets server ip");
        exit(EXIT_FAILURE);
    }
    if (fgets(server_stream_port, INET_ADDRSTRLEN, f) == NULL) {
        perror("CLIENT::ERROR:: fgets server stream port");
        exit(EXIT_FAILURE);
    }
    if (fgets(server_dgram_port, INET_ADDRSTRLEN, f) == NULL) {
        perror("CLIENT::ERROR:: fgets server dgram port");
        exit(EXIT_FAILURE);
    }
    fclose(f);

    // strip new line characters
    server_ip[strlen(server_ip) - 1] = '\0';
    server_stream_port[strlen(server_stream_port) - 1] = '\0';

    return 0;
}

int main(int argc, char** argv)
{
    int sock;
    int ecode;
    int query_mode = 0;
    int retval;
    struct addrinfo hints, *addrlist;
    struct sockaddr_in *server;
    long timeout = -1;
    char *endptr;

    if (argc > 1) {
        if (strncmp(argv[1], "-q", 2) == 0) {
            query_mode = 1;
        } else if (strncmp(argv[1], "-t", 2) == 0) {
            if (argc != 3) {
                print_usage();
                exit(EXIT_FAILURE);
            }
            timeout = strtol(argv[2], &endptr, 10);
            if (errno != 0 || *endptr != '\0' || timeout < 0) {
                fprintf(stderr, "Incorrect timeout argument: %s\n", argv[2]);
                print_usage();
                exit(EXIT_FAILURE);
            }
        }
    }

#ifdef DEBUG
    fprintf(stderr, "CLIENT::DEBUG:: timeout = %ld\n", timeout);
#endif /* DEBUG */

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = query_mode ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    char server_ip[INET_ADDRSTRLEN];
    char server_stream_port[10];
    char server_dgram_port[10];
    if (get_server_info(server_ip, server_stream_port, server_dgram_port) == -1) {
#ifdef DEBUG
        fprintf(stderr, "CLIENT::DEBUG:: server_addr.txt does not exist. Retrying in 60 seconds.\n");
#endif /* DEBUG */
        sleep(60);
#ifdef DEBUG
        fprintf(stderr, "CLIENT::DEBUG:: Retrying to access server_addr.txt.\n");
#endif /* DEBUG */
        if (get_server_info(server_ip, server_stream_port, server_dgram_port) == -1) {
            fprintf(stderr, "server_addr.txt still doesn't exist. Quitting client.\n");
            exit(EXIT_FAILURE);
        }
    }

    ecode = getaddrinfo(server_ip, query_mode ? server_dgram_port : server_stream_port,
            &hints, &addrlist);
    if (ecode != 0) {
        fprintf(stderr, "CLIENT::ERROR:: getaddrinfo: %s\n", gai_strerror(ecode));
        exit(EXIT_FAILURE);
    }

    server = (struct sockaddr_in*)addrlist->ai_addr;

    sock = socket(addrlist->ai_family, addrlist->ai_socktype, 0);
    if (sock < 0) {
        perror("CLIENT::ERROR:: socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)server, sizeof(struct sockaddr_in)) < 0) {
        perror("CLIENT::ERROR:: connect");
        exit(EXIT_FAILURE);
    }

    if (query_mode) {
        struct MsgDgram msg;
        send(sock, &msg, sizeof(msg), 0);
        recv(sock, &msg, sizeof(msg), 0);
        if (msg.nplayers == 2) {
            printf("There is a game in progress\n"
                   "player 1: %s\n"
                   "player 2: %s\n",
                   msg.player1, msg.player2);
        } else if (msg.nplayers == 1) {
            printf("There is one player connected\n"
                   "player 1: %s\n",
                   msg.player1);
        } else {
            printf("There are no players connected\n");
        }
        return 0;
    }

    struct Message msg;
    char handle[MAX_HANDLE_LEN];
    char opp_handle[MAX_HANDLE_LEN];
    char xo;
    char board[10];
    int move;


    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    struct timeval *tv = NULL;
    if (timeout > -1) {
        struct timeval t;
        t.tv_sec = timeout;
        t.tv_usec = 0;
        tv = &t;
    }
    retval = select(FD_SETSIZE, &rfds, NULL, NULL, tv);

    if (retval == -1) {
        perror("CLIENT::ERROR:: select");
        exit(EXIT_FAILURE);
    } else if (retval == 0) {
        printf("Connection attempt timed out\n");
        return 0;
    }


    // read WHO req
    if (read_message(sock, &msg) == -1) {
        printf("Server disconnected\nQuitting client\n");
        exit(EXIT_FAILURE);
    }
    if (msg.type == WHO) {
        // respond to WHO req
        // request handle
        printf("Enter a handle: ");
        scanf("%s", handle);
        msg.msg_str = handle;
        write_message(sock, &msg);
    } else {
        fprintf(stderr, "CLIENT::ERROR:: Expected WHO message\n");
        exit(EXIT_FAILURE);
    }

    // read opponent handle
    msg.msg_str = opp_handle;
    if (read_message(sock, &msg) == -1) {
        printf("Server disconnected\nQuitting client\n");
        exit(EXIT_FAILURE);
    }
    if (msg.type != OPP_HANDLE) {
        fprintf(stderr, "CLIENT::ERROR:: Expected OPP_HANDLE message");
        exit(EXIT_FAILURE);
    }

    printf("You are playing: %s\n", opp_handle);

    // read XO
    if (read_message(sock, &msg) == -1) {}
    if (msg.type == XO) {
        xo = msg.msg_char;
    } else {
        fprintf(stderr, "CLIENT::ERROR:: Expected XO message\n");
        exit(EXIT_FAILURE);
    }

    printf("You are playing as: %c\n", xo);

    while  (1) {
        // read move request
        msg.msg_str = board;
        if (read_message(sock, &msg) == -1) {
            printf("Server disconnected\nQuitting client\n");
            exit(EXIT_FAILURE);
        }
        if (msg.type != MOVE && msg.type != RESULT) {
            fprintf(stderr, "CLIENT:: Expected MOVE or RESULT message. Received %c\n", msg.type);
            exit(EXIT_FAILURE);
        }

        print_board(board);

        if (msg.type == RESULT) {
            if (msg.msg_char == xo) { // you won
                printf("You won\n");
            } else if (msg.msg_char == ' ') { // you tied
                printf("You tied\n");
            } else { // you lost
                printf("You lost\n");
            }
            return 0;
        }

        // prompt user for move
        printf("MOVE: ");
        scanf("%d", &move);

        move--; // translate move into board index

        if (move < 0 || move > 8) {
            fprintf(stderr, "CLIENT:: Invalid move. "
                            "Move must be an integer between 0 and 8 (inclusive)\n");
            exit(EXIT_FAILURE);
        }

        if (board[move] != ' ') {
            fprintf(stderr, "CLIENT:: Invalid move. "
                            "This position is already occupied\n");
            exit(EXIT_FAILURE);
        }

        board[move] = xo;

        print_board(board);

        // respond to move req
        msg.type = MOVE;
        msg.msg_int = move;
        msg.msg_str = NULL; // !important
        write_message(sock, &msg);
    }
}

