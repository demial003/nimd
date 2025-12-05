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
    int turn;
    int order;
    char board[10];
} Player;

#define MESSAGE_LEN_MAX 104

int convertString(char *num);
int boardEmpty(char* board);
void applyMove(int pile, int amount, char *board);

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

    char seg[3];
    seg[0] = inbox[2];
    seg[1] = inbox[3];
    seg[2] = '\0';
    int msglen = convertString(seg);
    int total = 5 + msglen;
    if (len >= total)
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

Message *readLine(int client_fd)
{
    char buf[MESSAGE_LEN_MAX + 1];
    char inbox[8192];
    int boxlen = 0;
    while (1)
    {

        int bytes = read(client_fd, buf, MESSAGE_LEN_MAX);

        if (bytes == 0)
        {
            return NULL;
        }

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
    Message *m = readLine(fd);
    if (m != NULL && strcmp(m->type, "OPEN") == 0)
    {
        player->fd = fd;
        strcpy(player->name, m->fields[0]);
        send(player->fd, "0|05|WAIT|\n", 11, 0);
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
    if (strcmp(m->type, "MOVE") == 0)
    {
        int pile = convertString(m->fields[0]);
        int amount = convertString(m->fields[1]);

        if (!p1->turn)
        {
            char *err = "31 Impatient";
            exit(1);
        }

        applyMove(pile, amount, p2->board);
        if (boardEmpty(p2->board))
        {
            char*message = malloc(23);
            snprintf(message, 33, "0|18|OVER|%d|%s||\n", p2->order, p2->board);
            send(p2->fd, message, strlen(message), 0);
            send(p1->fd, message, strlen(message), 0);
            free(message);

            exit(1);
        }
        else{
            char* message = malloc(22);
            snprintf(message, 33, "0|17|PLAY|%d|%s|\n", p1->order, p1->board);
            send(p2->fd, message, strlen(message), 0);
            free(message);
        }
        p1->turn = 0;
        p2->turn = 1;
    }
}

void applyMove(int pile, int amount, char *board)
{

    if (pile > 5)
    {
        char *err = "31 Pile Index";
    }
    if (amount > 9)
    {
        char *err = "33 Quanity";
    }

    int idx = pile * 2 - 2;
        if(boardEmpty(board) || board[idx] == '0'){
        return;
    }
    char *num = malloc(2);
    if(board[idx] <= '0'){
        return;
    }
    sprintf(num, "%c", board[idx]);
    int newAmt = amount - convertString(num);
    if (newAmt <= 0)
    {
        board[idx] = '0';
    }
    else
    {
        board[idx] = (char)newAmt;
    }
    return;
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
