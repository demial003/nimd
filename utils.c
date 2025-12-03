typedef struct{
    int version;
    int length;
    char type[5];
    char** fields;
    int fieldCount;
} Message;


typedef struct{
    int fd;
    char name[73];
} Player;

#define MESSAGE_LEN_MAX 104

int convertString(char* num);

Message * parse(char* raw, int bytes){
    Message *m;
    char** tokens = malloc(2 * bytes);
    int tokenCount = 0;
    int prev = 0;
    for(int pos = 0; pos < bytes; pos++){
        char c = raw[pos];
        if(c == '|'){
            int len = pos - prev;
            char* s = malloc(len + 1);
            memcpy(s, raw + prev + 1, len - 1);
            s[len] = '\0';
            tokens[tokenCount] = s;
            ++tokenCount;
            prev = pos;
        }
    }
    char v[2];
    v[0] = raw[0];
    v[1] = '\0';
    m->version = convertString(v);
    m->length = convertString(tokens[1]);
    if(strlen(tokens[2]) != 4) return NULL;
    memcpy(m->type, tokens[2], 5);
    m->fieldCount = tokenCount - 3;
    m->fields = malloc(2 * m->fieldCount);
    for(int i = 0; i < m->fieldCount; i++){
        m->fields[i] = tokens[3 + i];
    }
    return m;
}


int extract_message(char* inbox, int len){
    int msglen = 1;
    if(len < 5) return -1;

    char seg[3];
    char* endptr;
    memcpy(seg, inbox + 2, 2);
    seg[2] = '\0';
    msglen = (int) strtol(seg, &endptr, 10);
    if(len >= 5 + msglen){
        return 5 + msglen;
    }
    return -1;
}

int convertString(char* num){
    char* endptr;
    return (int) strtol(num, &endptr, 10);
}

Message* readLine(int client_fd){
    char buf[MESSAGE_LEN_MAX + 1];
    char inbox[8192];
    int boxlen = 0;
    while (1)
    {
        
        int  bytes = read(client_fd, buf, MESSAGE_LEN_MAX);
          
        if (bytes == 0)
        {
            fprintf(stderr, "Disconnected\n");
            break;
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
                Message* m = parse(message, n);
                return m;
            }
            else{
                break;
            }
        }
    }
}

void destroyMessage(Message *m){
    for(int i = 0; i < m->fieldCount; i++){
        free(m->fields[i]);
    }
    free(m->fields);
}

void initGame(){

}


