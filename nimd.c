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
    Player* player1 = malloc(sizeof(Player));
    Message* m = readLine(client1_fd);
    if(m != NULL && strcmp(m->type, "OPEN") == 0){
        player1->fd = client1_fd;
        strcpy(player1->name, m->fields[0]);
        send(player1->fd, "0|05|WAIT|", 11, 0);
        puts(player1->name);
    }

    int client2_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (client2_fd < 0)
    {
        perror("connect client2");
        exit(1);
    }
    printf("Client2 connected\n");
    Player* player2 = malloc(sizeof(Player));
    Message* m2 = readLine(client2_fd);
    if(m2 != NULL && strcmp(m2->type, "OPEN") == 0){
        player2->fd = client2_fd;
        strcpy(player2->name, m2->fields[0]);
        send(player2->fd, "0|05|WAIT|", 11, 0);
        puts(player2->name);
    }


    close(server_fd);
    close(client1_fd);

    return 0;
}