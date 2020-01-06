#include <stdlib.h>
#include "threadpool.h"
#include "err.h"
#include <stdio.h>
#include <signal.h>


//global static structure for SIGINT catch
static pthread_mutex_t pool_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static node_t pool_list = {NULL, NULL, NULL};

void execute(runnable_t *task) {
    void (*fun)(void *, size_t);
    fun = task->function;
    void *arg = task->arg;
    size_t args = task->argsz;
    (*fun)(arg, args);
}

void *worker(void *data) {
    thread_pool_t *pool = (thread_pool_t *) data;
    queue_t *queue = pool->queue;
    
    while (1) {
        if (pthread_mutex_lock(&queue->mutex) != 0)
            syserr("queue mutex lock");
        
        while (is_empty(queue) && !pool->end) {
            if (pthread_cond_wait(&queue->waiting, &queue->mutex) != 0)
                syserr("queue cond wait");
        }
        
        if (pool->end && is_empty(queue)) { //end variable is set, but exist tasks in queue
            if (pthread_mutex_unlock(&queue->mutex) != 0)
                syserr("queue mutex unlock");
            break;
        }
        
        runnable_t *task = (runnable_t *) dequeue(queue);
        
        if (pthread_mutex_unlock(&queue->mutex) != 0)
            syserr("queue mutex unlock");
        
        execute(task);
        free(task);
    }
    
    return NULL;
}

//without delete self from global pool_list
void pool_structure_destroy(thread_pool_t *pool) {
    pool->end = 1;
    void **res = NULL;
    
    if (pthread_mutex_lock(&pool->queue->mutex) != 0)
        syserr("queue mutex lock failed");
    
    if (pthread_cond_broadcast(&pool->queue->waiting) != 0)
        syserr("queue mutex lock failed");
    
    if (pthread_mutex_unlock(&pool->queue->mutex) != 0)
        syserr("queue mutex unlock failed");
    
    for (size_t i = 0; i < pool->pool_size; i++) {
        if (pthread_join(pool->threads[i], res) != 0)
            syserr("join pthread");
    }
    destroy_queue(pool->queue);
    free(pool->threads);
}

void turn_off_signal_catch(){
    signal(SIGINT,SIG_DFL);
}


void sig_handler(int sig) {
    node_t *iter = pool_list.next;
    
    printf("sigint catch\n");
    if (pthread_mutex_lock(&pool_list_mutex) != 0)
        syserr("list mutex lock failed");
    
    node_t *to_destroy;
    while (iter != NULL) {
        pool_structure_destroy(iter->item);
        to_destroy = iter;
        iter = iter->next;
        free(to_destroy);
    }
    
    pool_list.next = NULL;
    turn_off_signal_catch();
    
    if (pthread_mutex_unlock(&pool_list_mutex) != 0)
        syserr("list mutex unlock failed");
    
}

void turn_on_signal_catch(){
    signal(SIGINT, sig_handler);
}


node_t *add_to_pool_list(thread_pool_t *pool) {
    node_t *node = malloc(sizeof(node_t));
    node->item = pool;
    
    if (pthread_mutex_lock(&pool_list_mutex) != 0)
        syserr("queue mutex lock failed");
    
    node->prev = &pool_list;
    node->next = pool_list.next;
    pool_list.next = node;
    
    if (node->next != NULL) {
        node->next->prev = node;
    } else{ //first pool on the list turn on signal catch
        turn_on_signal_catch();
    }
    
    if (pthread_mutex_unlock(&pool_list_mutex) != 0)
        syserr("queue mutex unlock failed");
    
    return node;
}


void remove_from_pool_list(node_t *node) {
    if (pthread_mutex_lock(&pool_list_mutex) != 0)
        syserr("queue mutex lock failed");
    
    node->prev->next = node->next;
    
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    
    if(pool_list.next == NULL) { //list is empty
        turn_off_signal_catch();
    }
    
    if (pthread_mutex_unlock(&pool_list_mutex) != 0)
        syserr("queue mutex unlock failed");
    
    free(node);
}


int thread_pool_init(thread_pool_t *pool, size_t num_threads) {
    pthread_t *th = malloc(sizeof(pthread_t) * num_threads);
    pthread_attr_t attr;
    
    if (pthread_attr_init(&attr) != 0)
        syserr("attr_init");
    
    pool->queue = init_queue();
    pool->end = 0;
    pool->pool_size = num_threads;
    pool->threads = th;
    
    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&th[i], &attr, worker, pool) != 0)
            syserr("create thread");
    }
    
    pool->pool_list_element = add_to_pool_list(pool);
    
    return 0;
}

void thread_pool_destroy(struct thread_pool *pool) {
    if(!pool->end) { //not active thread_pool
        remove_from_pool_list(pool->pool_list_element);
        pool_structure_destroy(pool);
    }
}

int defer(struct thread_pool *pool, runnable_t runnable) {
    if (pool->end) {
        return -1;
    }
    
    runnable_t *task = malloc(sizeof(runnable_t));
    *task = runnable;
    int res = enqueue(pool->queue, task);
    
    return res;
}
