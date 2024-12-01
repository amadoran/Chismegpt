#include "message_structs.h"
#include <stdlib.h>

typedef struct Node
{
    Message data;
    struct Node *next;
} Node;

Node *createNode(Message new_data)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->data = new_data;
    new_node->next = NULL;
    return new_node;
}

typedef struct Queue
{
    Node *front, *rear;
} Queue;

Queue *createQueue()
{
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

int isEmpty(Queue *q)
{
    if (q->front == NULL && q->rear == NULL)
    {
        return 1;
    }
    return 0;
}

void enqueue(Queue *q, Message new_data)
{
    Node *new_node = createNode(new_data);
    if (q->rear == NULL)
    {
        q->front = q->rear = new_node;
        return;
    }
    q->rear->next = new_node;
    q->rear = new_node;
}

Message dequeue(Queue *q)
{
    if (isEmpty(q))
    {
        Message_Text msg_txt = {0, 0, 0, " "};
        Message empty = {0, msg_txt};
        return empty;
    }
    Node *temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL)
        q->rear = NULL;

    Message data = temp->data;
    free(temp);
    return data;
}

void push_front(Queue *q, Message new_data)
{
    Node *new_node = createNode(new_data);
    if (q->rear == NULL)
    {
        q->front = q->rear = new_node;
        return;
    }

    new_node->next = q->front;
    q->front = new_node;
}

Message *getFront(Queue *q)
{
    if (isEmpty(q))
    {
        return NULL;
    }
    return &q->front->data;
}

Message *getRear(Queue *q)
{
    if (isEmpty(q))
    {
        return NULL;
    }
    return &q->rear->data;
}