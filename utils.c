#include <errno.h>
typedef struct
{
    int version;
    int length;
    char type[5];
    char **fields;
    int fieldCount;

} Message;

typedef struct
{
    int fd;
    char name[73];
    int ready;
    int inGame;
    int turn;
    int order;
    char board[10];
    char inbox[8192];
    int boxlen;
} Player;

#define MESSAGE_LEN_MAX 104

int convertString(char *num);
int boardEmpty(char *board);
void applyMove(int pile, int amount, char *board);
char *build_message(const char *body);
void sendFail(int fd, int code, const char *msgText, int closeAfter);

Message *parse(char *raw, int bytes)
{

    int maxFields = 0;
    for (int i = 0; i < bytes; ++i)
    {
        if (raw[i] == '|')
        {
            ++maxFields;
        }
    }
    if (maxFields < 3)
    {
        return NULL;
    }

    char **tokens = malloc(sizeof(char *) * (maxFields + 1));
    int tokenCount = 0;
    int prev = 0;
    for (int pos = 0; pos < bytes; pos++)
    {
        char c = raw[pos];
        if (c == '|')
        {
            int len = pos - prev;
            char *s = malloc(len + 1);
            memcpy(s, raw + prev, len);
            s[len] = '\0';
            tokens[tokenCount++] = s;
            prev = pos + 1;
        }
    }

    Message *m = malloc(sizeof(Message));
    m->version = convertString(tokens[0]);
    m->length = convertString(tokens[1]);
    if ((int)strlen(tokens[2]) != 4)
    {
        return NULL;
    }
    strncpy(m->type, tokens[2], 5);
    m->type[4] = '\0';

    int rem = tokenCount - 3;
    m->fieldCount = rem;
    m->fields = malloc(sizeof(char *) * rem);
    for (int i = 0; i < rem; i++)
    {
        m->fields[i] = tokens[3 + i];
    }

    return m;
}

int extract_message(char *inbox, int len)
{
    if (len < 5)
        return -1;

    if (inbox[0] != '0' || inbox[1] != '|')
        return -1;

    char seg[3];
    seg[0] = inbox[2];
    seg[1] = inbox[3];
    seg[2] = '\0';
    int msglen = convertString(seg);
    if (msglen < 0)
        return -1;

    int total = 5 + msglen;
    if (len >= total && inbox[total - 1] == '|')
    {
        return total;
    }
    return -1;
}

int convertString(char *num)
{
    char *endptr;
    return (int)strtol(num, &endptr, 10);
}

Message *readLine(Player *player)
{
    char buf[MESSAGE_LEN_MAX + 1];
    int client_fd = player->fd;
    while (1)
    {

        int bytes = read(client_fd, buf, MESSAGE_LEN_MAX);

        if (bytes == 0)
        {
            return NULL;
        }
        if (bytes < 0)
        {
            if (errno == EINTR)
                continue;
            perror("read");
            return NULL;
        }

        memcpy(player->inbox + player->boxlen, buf, bytes);
        player->boxlen += bytes;
        int n;
        while (player->boxlen > 0)
        {
            n = extract_message(player->inbox, player->boxlen);
            if (n > 0)
            {
                char message[n + 1];
                memmove(message, player->inbox, n);
                message[n] = '\0';
                memmove(player->inbox, player->inbox + n, player->boxlen - n);
                player->boxlen -= n;
                player->inbox[player->boxlen] = '\0';
                Message *m = parse(message, n);
                return m;
            }
            else
            {
                break;
            }
        }
    }
    return NULL;
}

void destroyMessage(Message *m)
{
    if (!m)
    {
        return;
    }
    if (m->fields)
    {
        for (int i = 0; i < m->fieldCount; ++i)
        {
            if (m->fields[i])
                free(m->fields[i]);
        }
        free(m->fields);
    }
    free(m);
}

Player *createPLayer(int fd)
{
    Player *player = malloc(sizeof(Player));
    memset(player, 0, sizeof(Player));
    player->fd = fd;
    player->boxlen = 0;
    Message *m = readLine(player);
    if (strlen(m->fields[0]) > 72)
    {
        sendFail(fd, 21, "Long Name", 1);
        destroyMessage(m);
        free(player);
        return NULL;
    }
    if (m != NULL && strcmp(m->type, "OPEN") == 0)
    {
        strcpy(player->name, m->fields[0]);
        char *waitmsg = build_message("WAIT|");
        if (waitmsg)
        {
            send(player->fd, waitmsg, strlen(waitmsg), 0);
            free(waitmsg);
        }

        player->ready = 1;
        char *board = "1 3 5 7 9";
        strncpy(player->board, board, 10);
        destroyMessage(m);
        return player;
    }
    destroyMessage(m);
    return NULL;
}
void handleMessage(Player *p1, Player *p2, Message *m)
{
    if (strcmp(m->type, "OPEN") == 0 && p1->ready)
    {
        sendFail(p1->fd, 23, "Already Open", 1);
        return;
    }

    if (strcmp(m->type, "MOVE") == 0 && !p1->ready)
    {
        sendFail(p1->fd, 24, "Not Playing", 1);
        return;
    }

    if (strcmp(m->type, "MOVE") == 0)
    {
        int pile = convertString(m->fields[0]);
        int amount = convertString(m->fields[1]);

        if (pile < 1 || pile > 5)
        {
            sendFail(p1->fd, 32, "Pile Index", 0);
            return;
        }
        if (amount < 1 || amount > 9)
        {
            sendFail(p1->fd, 33, "Quantity", 0);
            return;
        }

        if (!p1->turn)
        {
            sendFail(p1->fd, 31, "Impatient", 0);
            return;
        }

        applyMove(pile, amount, p2->board);
        strcpy(p1->board, p2->board);
    }

    if (boardEmpty(p2->board))
    {
        char body[64];
        snprintf(body, sizeof(body), "OVER|%d|%s||", p1->order, p2->board);
        char *message = build_message(body);
        send(p2->fd, message, strlen(message), 0);
        send(p1->fd, message, strlen(message), 0);
        free(message);

        close(p1->fd);
        close(p2->fd);
        return;
    }
    else
    {
        char body1[64];
        char body2[64];

        snprintf(body1, sizeof(body1), "PLAY|%d|%s|", p2->order, p2->board);
        snprintf(body2, sizeof(body2), "PLAY|%d|%s|", p2->order, p2->board);
        char *message1 = build_message(body1);
        char *message2 = build_message(body2);
        send(p2->fd, message1, strlen(message1), 0);
        send(p1->fd, message2, strlen(message2), 0);
        free(message1);
        free(message2);
    }
    p1->turn = 0;
    p2->turn = 1;
}

void applyMove(int pile, int amount, char *board)
{

    int idx = (pile - 1) * 2;
    if (boardEmpty(board) || board[idx] == '0')
    {
        return;
    }

    int cur = board[idx] - '0';
    int newAmt = cur - amount;
    if (newAmt <= 0)
    {
        board[idx] = '0';
    }
    else
    {
        board[idx] = '0' + newAmt;
    }
}

int boardEmpty(char *board)
{
    for (int i = 0; i < 9; i += 2)
    {
        if (board[i] != '0')
        {
            return 0;
        }
    }
    return 1;
}

char *build_message(const char *body)
{
    int body_len = strlen(body);
    if (body_len > 104)
        return NULL;
    char header[16];
    snprintf(header, sizeof(header), "0|%02d|", body_len);
    int total = strlen(header) + body_len;
    char *msg = malloc(total + 1);
    if (!msg)
        return NULL;
    memcpy(msg, header, strlen(header));
    memcpy(msg + strlen(header), body, body_len);
    msg[total] = '\0';
    return msg;
}

void sendFail(int fd, int code, const char *msgText, int closeAfter)
{
    char body[128];
    snprintf(body, sizeof(body), "FAIL|%d %s|", code, msgText ? msgText : "");
    char *msg = build_message(body);
    if (msg)
    {
        send(fd, msg, strlen(msg), 0);
        free(msg);
    }
    if (closeAfter)
    {
        close(fd);
    }
}
