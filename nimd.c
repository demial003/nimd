#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "utils.c"

#define MESSAGE_LEN_MAX 104

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

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen);
    if (client_fd < 0)
    {
        perror("connect");
        exit(1);
    }
    printf("Client connected\n");

    char buf[MESSAGE_LEN_MAX + 1];
    char inbox[8192];
    int boxlen = 0;
    while (1)
    {
        int bytes = read(client_fd, buf, MESSAGE_LEN_MAX);
        if (bytes == 0)
        {
            fprintf(stderr, "Disconnected\n");
            break;
        }
        if (bytes < 0)
        {
            perror("read");
            break;
        }
        buf[bytes] = '\0';

        memcpy(inbox + boxlen, buf, bytes);
        boxlen += bytes;

        int n;
        while (boxlen > 0)
        {
            n = extract_message(inbox, boxlen);
            if (n > 0)
            {
                char message[n + 1];
                memmove(message, inbox, n);
                message[n] = '\0';
                memmove(inbox, inbox + n, boxlen - n);
                boxlen -= n;
                inbox[boxlen] = '\0';
                printf("Message: %s\n", message);
            }
            else{
                break;
            }
        }
        // printf("%d\n", n);
    }

    close(server_fd);
    close(client_fd);

    return 0;
}