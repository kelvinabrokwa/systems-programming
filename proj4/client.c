/**
 * client.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include "msg.h"

int main(int argc, char** argv) {
    int sock;
    int ecode;
    struct addrinfo hints, *addrlist;
    struct sockaddr_in *server;
    int query_mode = 0;

    if (argc > 1 && strncmp(argv[1], "-q", 2) == 0) {
        query_mode = 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = query_mode ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    // read server information from file
    FILE *f = fopen("server_addr.txt", "r");
    if (f == NULL) {
        perror("fopen");
        fprintf(stderr, "CLIENT::ERROR:: Could not ope server_addr.txt file\n");
        exit(EXIT_FAILURE);
    }
    char server_ip[INET_ADDRSTRLEN];
    char server_stream_port[10];
    char server_dgram_port[10];
    if (fgets(server_ip, INET_ADDRSTRLEN, f) == NULL) {
        perror("CLIENT::ERROR:: fgets");
        exit(EXIT_FAILURE);
    }
    if (fgets(server_stream_port, INET_ADDRSTRLEN, f) == NULL) {
        perror("CLIENT::ERROR:: fgets");
        exit(EXIT_FAILURE);
    }
    if (fgets(server_dgram_port, INET_ADDRSTRLEN, f) == NULL) {
        perror("CLIENT::ERROR:: fgets");
        exit(EXIT_FAILURE);
    }
    fclose(f);

    // strip new line characters
    server_ip[strlen(server_ip) - 1] = '\0';
    server_stream_port[strlen(server_stream_port) - 1] = '\0';

    ecode = getaddrinfo(server_ip, query_mode ? server_dgram_port : server_stream_port, &hints, &addrlist);
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

    // request handle
    printf("Enter a handle: ");
    scanf("%s", handle);

    // read WHO req
    read_message(sock, &msg);
    if (msg.type == WHO) {
        // respond to WHO req
        //strncpy(msg.msg_str, handle, MAX_HANDLE_LEN);
        msg.msg_str = handle;
        write_message(sock, &msg);
    } else {
        fprintf(stderr, "CLIENT::ERROR:: Expected WHO message\n");
        exit(EXIT_FAILURE);
    }

    // read opponent handle
    read_message(sock, &msg);
    if (msg.type == OPP_HANDLE) {
        strncpy(opp_handle, msg.msg_str, MAX_HANDLE_LEN);
    } else {
        fprintf(stderr, "CLIENT:: Expected OPP_HANDLE message");
        exit(EXIT_FAILURE);
    }

    printf("You are playing: %s\n", opp_handle);

    // read XO
    read_message(sock, &msg);
    if (msg.type == XO) {
        xo = msg.msg_char;
    } else {
        fprintf(stderr, "CLIENT:: Expected XO message\n");
        exit(EXIT_FAILURE);
    }

    printf("You are playing as: %c\n", xo);

    while  (1) {
        // read move request
        read_message(sock, &msg);
        if (msg.type != MOVE && msg.type != RESULT) {
            fprintf(stderr, "CLIENT:: Expected MOVE message\n");
            exit(EXIT_FAILURE);
        }

        strncpy(board, msg.msg_str, 9);

        print_board(board);

        // prompt user for move
        printf("MOVE: ");
        scanf("%d", &move);

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

