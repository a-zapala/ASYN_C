#include <stdlib.h>
#include "future.h"
#include "threadpool.h"
#include "err.h"

typedef struct {
    callable_t callable;
    future_t *future;
} execute_callable_data_t;

void init_future(future_t *future) {
    future->is_ready = 0;
    future->is_mapped = 0;
    
    if (pthread_mutex_init(&future->mutex, 0) != 0)
        syserr("future mutex init failed");
    
    if (pthread_cond_init(&future->waiting, NULL) != 0)
        syserr("future cond init failed");
}

void destroy_future(future_t *future) {
    
    if (pthread_cond_destroy(&future->waiting) != 0)
        syserr("future cond destroy failed");
    
    if (pthread_mutex_destroy(&future->mutex) != 0)
        syserr("future mutex destroy failed");
}


void set_ready_status(future_t *fut) {
    if (pthread_mutex_lock(&fut->mutex) != 0)
        syserr("future mutex lock");
    
    fut->is_ready = 1;
    
    if (pthread_cond_signal(&fut->waiting) != 0)
        syserr("cond signal failed");
    
    if (pthread_mutex_unlock(&fut->mutex) != 0)
        syserr("future mutex unlock");
}

void async_mapped_function(future_t *fut);

//function execute callable object using interface for runnable object
void execute_callable(void *data, size_t size) {
    execute_callable_data_t *execute_callable = (execute_callable_data_t *) data;
    
    void *(*fun)(void *, size_t, size_t *);
    fun = execute_callable->callable.function;
    void *arg = execute_callable->callable.arg;
    size_t args = execute_callable->callable.argsz;
    future_t *fut = execute_callable->future;
    
    fut->result = (*fun)(arg, args, &fut->size);
    
    set_ready_status(fut);
    
    //if task has continuation
    if (fut->is_mapped) {
        async_mapped_function(fut);
        destroy_future(fut);
    }
    
    free(execute_callable);
}

int
async_without_initialization_future(thread_pool_t *pool, future_t *future, callable_t callable) {
    execute_callable_data_t *data = (execute_callable_data_t *) malloc(
            sizeof(execute_callable_data_t));
    
    data->future = future;
    data->callable = callable;
    
    runnable_t task;
    task.function = execute_callable;
    task.arg = data;
    
    return defer(pool, task);
}

void async_mapped_function(future_t *fut) {
    callable_t callable;
    callable.arg = fut->result;
    callable.argsz = fut->size;
    callable.function = fut->function;
    
    //future has been initialized in map()
    async_without_initialization_future(fut->mapped_pool, fut->mapped_value, callable);
}


int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    init_future(future);
    return async_without_initialization_future(pool, future, callable);
}

int map(thread_pool_t *pool, future_t *future, future_t *from,
        void *(*function)(void *, size_t, size_t *)) {
    
    if (pthread_mutex_lock(&from->mutex) != 0)
        syserr("future mutex lock");
    
    if (from->is_ready == 1) { //this thread put task to thread_pool
        callable_t callable;
        callable.arg = from->result;
        callable.argsz = from->size;
        callable.function = function;
        
        if (pthread_mutex_unlock(&from->mutex) != 0)
            syserr("future mutex unlock");
        destroy_future(from);
        
        return async(pool, future, callable);
    } else { //thread, which   will execute future_t from, will put new task to thread_pool
        from->is_mapped = 1;
        from->mapped_value = future;
        from->function = function;
        from->mapped_pool = pool;
        
        init_future(future); //want to initialize mutex and cond, for waiting on future_t purpose
        
        if (pthread_mutex_unlock(&from->mutex) != 0)
            syserr("future mutex unlock");
        return 0;
    }
}

void *await(future_t *future) {
    
    if (pthread_mutex_lock(&future->mutex) != 0)
        syserr("future mutex lock");
    
    while (future->is_ready == 0) {
        if (pthread_cond_wait(&future->waiting, &future->mutex) != 0)
            syserr("queue cond wait");
    }
    
    if (pthread_mutex_unlock(&future->mutex) != 0)
        syserr("future mutex unlock");
    
    destroy_future(future);
    return future->result;
}
