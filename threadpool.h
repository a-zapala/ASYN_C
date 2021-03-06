#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include "queue.h"

typedef struct runnable {
  void (*function)(void *, size_t);
  void *arg;
  size_t argsz;
} runnable_t;

typedef struct thread_pool {
    queue_t *queue;
    pthread_t *threads;
    size_t pool_size;
    
    node_t *pool_list_element;
    volatile int end;
    
} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);

#endif
