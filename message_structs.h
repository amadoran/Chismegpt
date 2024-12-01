#ifndef MESSAGE_STRUCTS
#define MESSAGE_STRUCTS

typedef struct Message_Text{
    int qid;
    int counter;
    int pid;
    char buf[200];
} Message_Text;

typedef struct Message{
    long message_type;
    Message_Text message_text;
} Message;

#endif