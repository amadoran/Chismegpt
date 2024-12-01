#ifndef QUEUE_ADT
#define QUEUE_ADT
#include "message_structs.h"

typedef struct Node
{
    Message data;
    struct Node * next;
} Node;

typedef struct Queue {
    Node *front, *rear;
} Queue;

Node* createNode(Message new_data);

Queue* createQueue();

int isEmpty(Queue* q);

void enqueue(Queue* q, Message new_data);

Message dequeue(Queue* q);

Message getFront(Queue* q);

Message getRear(Queue* q);

void push_front(Queue *q, Message new_data);

#endif