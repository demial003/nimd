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
} Player;

#define MESSAGE_LEN_MAX 104

int convertString(char *num);

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

void initGame()
{
}
