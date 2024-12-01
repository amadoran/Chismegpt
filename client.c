#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "message_structs.h"
#include "read_csv.h"

#define SRVR_KEY_PATH "/tmp/request_queue"
#define PROJECT_ID 'A'

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL)
    {
        perror("fopen");
        return 1;
    }
    key_t server_queue_key;
    int server_qid, myqid;
    Message my_message;

    // create my client queue for receiving messages from server
    if ((myqid = msgget(IPC_PRIVATE, 0666)) == -1)
    {
        perror("msgget: myqid");
        exit(1);
    }

    if ((server_queue_key = ftok(SRVR_KEY_PATH, PROJECT_ID)) == -1)
    {
        perror("ftok");
        exit(1);
    }

    if ((server_qid = msgget(server_queue_key, 0)) == -1)
    {
        perror("msgget: server_qid");
        exit(1);
    }

    my_message.message_text.qid = myqid;
    my_message.message_text.counter = 0;

    int row = 0;
    char buf[1024];
    while (fgets(buf, 1024, input_file) != NULL)
    {
        my_message.message_text.pid = 0;
        row++;
        if (row == 1)
            continue;
        char *sep_line[2];
        read_csv(buf, sep_line);
        int msg_type = atoi(sep_line[0]);
        strncpy(my_message.message_text.buf, sep_line[1], 198);
        my_message.message_type = msg_type;
        if (msg_type == 1)
        {
            my_message.message_text.counter = 0;
        }

        // remove newline from string
        int length = strlen(my_message.message_text.buf);
        if (my_message.message_text.buf[length - 1] == '\n')
            my_message.message_text.buf[length - 1] = '\0';

        // send message to server
        printf("Sending message: %s\n", my_message.message_text.buf);
        if (msgsnd(server_qid, &my_message, sizeof(Message_Text), 0) == -1)
        {
            perror("client: msgsnd");
            exit(1);
        }

        printf("Waiting response from server...\n");
        // read response from server
        if (msgrcv(myqid, &my_message, sizeof(Message_Text), 0, 0) == -1)
        {
            perror("client: msgrcv");
            exit(1);
        }

        // process return message from server
        printf("Message received from server: %s\n\n", my_message.message_text.buf);
    }
    // remove message queue
    if (msgctl(myqid, IPC_RMID, NULL) == -1)
    {
        perror("client: msgctl");
        exit(1);
    }

    printf("Client: bye\n");

    exit(0);
}
