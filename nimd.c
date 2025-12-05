#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <poll.h>
#include "utils.c"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Bad args\n");
        exit(1);
    }

    long port;
    char *endptr;
    port = strtol(argv[1], &endptr, 10);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        puts("bad");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    socklen_t addrlen = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %li\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));

    int client1_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (client1_fd < 0)
    {
        perror("connect client2");
        exit(1);
    }
    printf("Client1 connected\n");
    Player *player1 = createPLayer(client1_fd);
    if (player1 == NULL)
    {
        fprintf(stderr, "failed to create player");
        exit(1);
    }
    player1->turn = 1;
    player1->order = 1;

    int client2_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (client2_fd < 0)
    {
        perror("connect client2");
        exit(1);
    }
    printf("Client2 connected\n");
    Player *player2 = createPLayer(client2_fd);
    if (player2 == NULL)
    {
        fprintf(stderr, "failed to create player");
        exit(1);
    }
    player2->order = 2;

    if (player1->ready && player2->ready)
    {
        int messageLen1 = 8 + strlen(player2->name);
        int messageLen2 = 8 + strlen(player1->name);
        int totalLen1 = 7 + messageLen1;
        int totalLen2 = 7 + messageLen2;
        char *message1 = malloc(totalLen1);
        snprintf(message1, totalLen1, "0|%d|NAME|1|%s|\n", messageLen1, player2->name);
        char *message2 = malloc(totalLen2);
        snprintf(message2, totalLen2, "0|%d|NAME|2|%s|\n", messageLen2, player1->name);
        send(player2->fd, message1, totalLen1, 0);

        send(player1->fd, message2, totalLen2, 0);
        char *message3 = malloc(22);
        snprintf(message3, 33, "0|17|PLAY|%d|%s|\n", player2->order, player2->board);
        send(player1->fd, message3, strlen(message3), 0);
        free(message1);
        free(message2);
        free(message3);
    }


    struct pollfd fds[2];
    fds[0].fd = player1->fd;
    fds[0].events = POLLIN;
    fds[1].fd = player2->fd;
    fds[1].events = POLLIN;

    while (1)
    {
        int ready = poll(fds, 2, -1);
        if (ready < 0)
        {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN)
        {
            Message *m = readLine(player1->fd);
            if (m == NULL)
            {
                fprintf(stderr, "Client1 Disconnected\n");
                break;
            }
            handleMessage(player1, player2, m);
            destroyMessage(m);
        }

        if (fds[1].revents & POLLIN)
        {
            Message *m = readLine(player2->fd);
            if (m == NULL)
            {
                fprintf(stderr, "Client1 Disconnected\n");
                break;
            }
            handleMessage(player2, player1, m);
            destroyMessage(m);
        }
    }

    close(server_fd);
    close(client1_fd);
    close(client2_fd);
    free(player1);
    free(player2);

    return 0;
}