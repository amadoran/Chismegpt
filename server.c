#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include "message_structs.h"
#include "queue_adt.h"

#define SRVR_KEY_PATH "/tmp/request_queue"
#define PROJECT_ID 'A'
#define QUEUE_PERM 0666
#define PREPAGO 2
#define POSPAGO 1

void process_request(Message message, int sim_time);
int deleteChild(Message *running, int size, int pid);
int is_running_empty(Message *running, int size);
void create_process(Message *running, int limit, Message message, int *counter, int sim_time);
int get_free_running(Message *running, int size);

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <num_concurrencia> <tiempo_sim:ms>\n", argv[0]);
        return EXIT_FAILURE;
    }

    key_t msg_queue_key;
    int qid;
    FILE *server_file;
    Message message;

    Queue *pre_queue = createQueue();

    int limit = atoi(argv[1]);
    int tiempo = atoi(argv[2]);
    Message running[limit];
    int counter = 0;
    int status;
    int n;
    int finished_pid;

    for (int i = 0; i < limit; i++)
    {
        Message_Text msg_txt = {0, 0, 0, " "};
        Message dummy = {0, msg_txt};
        running[i] = dummy;
    }

    if ((server_file = fopen(SRVR_KEY_PATH, "w")) == NULL)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }
    fclose(server_file);

    if ((msg_queue_key = ftok(SRVR_KEY_PATH, PROJECT_ID)) == -1)
    {
        perror("ftok");
        return EXIT_FAILURE;
    }

    if ((qid = msgget(msg_queue_key, IPC_CREAT | QUEUE_PERM)) == -1)
    {
        perror("msgget");
        return EXIT_FAILURE;
    }

    printf("Server started!\n");
    printf("Listening...\n");

    while (1)
    {
        n = msgrcv(qid, &message, sizeof(Message_Text), -PREPAGO, IPC_NOWAIT);
        if (n > 0)
        {
            if (message.message_type == PREPAGO)
            {
                enqueue(pre_queue, message);
            }
            else
            {
                int replace = get_free_running(running, limit);
                if (replace < 0)
                {
                    int deleted = wait(&status);
                    int index = deleteChild(running, limit, deleted);
                    if (index < 0)
                    {
                        fprintf(stderr, "Child not deleted\n");
                        return EXIT_FAILURE;
                    }
                    replace = index;
                }

                if (running[replace].message_type == PREPAGO)
                {
                    if (running[replace].message_text.pid != 0)
                    {
                        kill(running[replace].message_text.pid, SIGSTOP);
                        push_front(pre_queue, running[replace]);
                    }
                }
                counter = replace;
                create_process(running, limit, message, &counter, tiempo);
            }
        }
        else if (n < 0)
        {
            if ((errno != ENOMSG))
            {
                perror("msgrcv");
                return EXIT_FAILURE;
            }
        }

        if ((finished_pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            deleteChild(running, limit, finished_pid);
        }
        if (is_running_empty(running, limit))
        {
            Message pre_message = dequeue(pre_queue);
            if (pre_message.message_text.pid > 0)
            {
                waitpid(pre_message.message_text.pid, &status, WNOHANG | WUNTRACED);
                if (WIFSTOPPED(status))
                {
                    kill(pre_message.message_text.pid, SIGCONT);
                    running[counter] = pre_message;
                    counter = (counter + 1) % limit;
                }
            }
            else if (pre_message.message_type != 0)
            {
                create_process(running, limit, pre_message, &counter, tiempo);
            }
        }
        struct timespec req;
        req.tv_sec = (tiempo / 2) / 1000;
        req.tv_nsec = ((tiempo / 2) % 1000) * 1000000L;
        nanosleep(&req, NULL);
        printf("Listening...\n");
    }
    return 0;
}

void create_process(Message *running, int limit, Message message, int *counter, int sim_time)
{
    int pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid == 0)
    {
        process_request(message, sim_time);
        exit(0);
    }
    else
    {
        message.message_text.pid = pid;
        running[*counter] = message;
        *counter = (*counter + 1) % limit;
    }
}

void process_request(Message message, int sim_time)
{
    printf("Procesando mensaje: %s\n", message.message_text.buf);
    struct timespec req;
    req.tv_sec = sim_time / 1000;
    req.tv_nsec = (sim_time % 1000) * 1000000L;
    nanosleep(&req, NULL);
    strcpy(message.message_text.buf, "Respuesta de Chismegpt");

    if (message.message_type == 2 && message.message_text.counter > 10)
    {
        strcpy(message.message_text.buf, "Limite alcanzado");
    }

    int client_qid = message.message_text.qid;
    message.message_text.counter = message.message_text.counter + 1;

    if (msgsnd(client_qid, &message, sizeof(Message_Text), 0) == -1)
    {
        perror("msgsnd: server to client");
        exit(1);
    }
}

int deleteChild(Message *running, int size, int pid)
{
    for (int i = 0; i < size; i++)
    {
        if (running[i].message_text.pid == pid)
        {
            running[i].message_text.pid = 0;
            return i;
        }
    }
    return -1;
}

int is_running_empty(Message *running, int size)
{
    int counter = 0;
    for (int i = 0; i < size; i++)
    {
        if (running[i].message_text.pid == 0)
        {
            counter++;
        }
    }
    return counter == size;
}

int get_free_running(Message *running, int size)
{
    for (int i = 0; i < size; i++)
    {
        int is_prepago = running[i].message_type == PREPAGO;
        int is_finished = running[i].message_text.pid == 0;
        if (is_prepago || is_finished)
        {
            return i;
        }
    }
    return -1;
}
