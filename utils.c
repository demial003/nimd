typedef struct{
    int version;
    int length;
    char* type;
    char* rest;
} Message;


struct Message * parse(char* raw, int bytes){

}


int extract_message(char* inbox, int len){
    int msglen= 0;
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


