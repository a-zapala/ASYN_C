#include <stdlib.h>
#include <assert.h>
#include "queue.h"
#include "threadpool.h"
#include "err.h"

queue_t *init_queue() {
    int err;
    queue_t *queue = (queue_t *) malloc(sizeof(queue_t));
    
    if (queue == NULL)
        syserr("can't allocate memory for queue");
    
    if ((err = pthread_mutex_init(&queue->mutex, 0)) != 0)
        syserr_err("queue mutex init failed", err);
    
    if ((err = pthread_cond_init(&queue->waiting, NULL)) != 0)
        syserr_err("queue cond init failed",err);
    
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void *dequeue(queue_t *queue) {
    node_t *node;
    runnable_t *ret;
    
    assert(is_empty(queue) == 0);
    
    node = queue->head;
    ret = node->item;
    queue->head = (queue->head)->prev;
    queue->size--;
    free(node);
    
    return ret;
}

void destroy_queue(queue_t *queue) {
    void *item;
    while (!is_empty(queue)) {
        item = dequeue(queue);
        free(item);
    }
    
    if (pthread_cond_destroy(&queue->waiting) != 0)
        syserr("queue cond destroy failed");
    
    if (pthread_mutex_destroy(&queue->mutex) != 0)
        syserr("queue mutex destroy failed");
    
    free(queue);
}

int enqueue(queue_t *queue, void *item) {
    
    if(queue == NULL) {
        return -1;
    }
    
    //error while locking mutex indicate on
    if (pthread_mutex_lock(&queue->mutex) != 0)
        return -1;
    
    node_t *node = (node_t *) malloc(sizeof(node_t));
    node->item = item;
    node->prev = NULL;
    
    /*the queue is empty*/
    if (queue->size == 0) {
        queue->head = node;
        queue->tail = node;
    } else {
        /*adding item to the end of the queue*/
        queue->tail->prev = node;
        queue->tail = node;
    }
    queue->size++;
    
    //signal to waiting thread
    if (pthread_cond_signal(&queue->waiting) != 0)
        syserr("cond signal failed");
    
    if (pthread_mutex_unlock(&queue->mutex) != 0)
        syserr("queue mutex unlock failed");
    
    return 0;
}


int is_empty(queue_t *queue) {
    assert(queue != NULL);
    
    if (queue->size == 0) {
        return 1;
    } else {
        return 0;
    }
}
