#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include "utils.c"

typedef struct
{
    Player *p1;
    Player *p2;
} Game;
void *gameThread(void *arg);

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

    Player *waiting = NULL;
    for (;;)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }
        // create player and run OPEN handshake (createPLayer does that)
        Player *p = createPLayer(client_fd);
        if (!p)
        {
            close(client_fd);
            continue;
        }

        if (!waiting)
        {
            waiting = p;
        }
        else
        {

            Game *g = malloc(sizeof(Game));
            g->p1 = waiting;
            g->p2 = p;
            waiting = NULL;

            if (g->p1->inGame)
            {
                sendFail(g->p1->fd, 22, "Already playing", 1);

                free(g->p2);
                free(g);
                continue;
            }
            if (g->p2->inGame)
            {
                sendFail(g->p2->fd, 22, "Already playing", 1);
                free(g->p1);
                free(g);
                continue;
            }

            g->p1->inGame = 1;
            g->p2->inGame = 1;

            pthread_t tid;
            if (pthread_create(&tid, NULL, gameThread, g) != 0)
            {
                perror("pthread_create");
                close(g->p1->fd);
                close(g->p2->fd);
                free(g->p1);
                free(g->p2);
                free(g);
                continue;
            }
            pthread_detach(tid);
        }
    }
}

void *gameThread(void *arg)
{
    Game *g = (Game *)arg;
    Player *player1 = g->p1;
    Player *player2 = g->p2;

    struct pollfd fds[2];
    fds[0].fd = player1->fd;
    fds[0].events = POLLIN;
    fds[1].fd = player2->fd;
    fds[1].events = POLLIN;

    player1->turn = 1;
    player1->order = 1;
    player1->ready = 1;

    player2->turn = 0;
    player2->order = 2;
    player2->ready = 1;

    char nameBody1[128];
    snprintf(nameBody1, sizeof(nameBody1), "NAME|1|%s|", player2->name);
    char *nameMsg1 = build_message(nameBody1);
    send(player1->fd, nameMsg1, strlen(nameMsg1), 0);
    free(nameMsg1);

    char nameBody2[128];
    snprintf(nameBody2, sizeof(nameBody2), "NAME|2|%s|", player1->name);
    char *nameMsg2 = build_message(nameBody2);
    send(player2->fd, nameMsg2, strlen(nameMsg2), 0);
    free(nameMsg2);

    char playBody[128];
    snprintf(playBody, sizeof(playBody), "PLAY|1|%s|", player1->board);

    char *playMsg = build_message(playBody);
    send(player1->fd, playMsg, strlen(playMsg), 0);
    send(player2->fd, playMsg, strlen(playMsg), 0);
    free(playMsg);

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
            Message *m = readLine(player1);
            if (m == NULL)
            {
                sendFail(player1->fd, 10, "Invalid Message", 1);
                break;
            }
            handleMessage(player1, player2, m);
            destroyMessage(m);
        }

        if (fds[1].revents & POLLIN)
        {
            Message *m = readLine(player2);
            if (m == NULL)
            {
                sendFail(player2->fd, 10, "Invalid Message", 1);
                break;
            }
            handleMessage(player2, player1, m);
            destroyMessage(m);
        }
    }

    close(player1->fd);
    close(player2->fd);
    free(player1);
    free(player2);
    free(g);
    pthread_exit(NULL);
    return NULL;
}
