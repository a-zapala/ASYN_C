#ifndef ASYNC_QUEUE_H
#define ASYNC_QUEUE_H

#include <pthread.h>

typedef struct Node_t {
    void *item;
    struct Node_t *prev;
    struct Node_t *next;
} node_t;

typedef struct Queue {
    node_t *head;
    node_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t waiting;
    size_t size;
} queue_t;

queue_t *init_queue();
void destroy_queue(queue_t *queue);
int enqueue(queue_t *queue, void *item);
void *dequeue(queue_t *queue);
int is_empty(queue_t* queue);

#endif //ASYNC_QUEUE_H
