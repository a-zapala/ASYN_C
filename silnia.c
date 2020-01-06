#include <stdio.h>
#include <stdlib.h>

#include "threadpool.h"
#include "future.h"

void *partial_multiplication(void *args, size_t argsz __attribute__((unused)),
                             size_t *res_size __attribute__((unused))) {
    long *partial_product = args;
    long *number = (long *) args + 1;
    
    *partial_product = (*partial_product) * (*number);
    *number += 1;
    
    return partial_product;
}

long count_factorial(int n, thread_pool_t *pool) {
    if (n < 1) {
        return 0;
    } else {
        future_t *futures = malloc(sizeof(future_t) * n);
        long *args = malloc(sizeof(long) * 2);
        *args = 1;
        *(args + 1) = 1;
        
        async(pool, futures,
              (callable_t) {.function = partial_multiplication, .arg = args, .argsz = sizeof(long) *
                                                                                      2});
        for (int i = 1; i < n; i++) {
            map(pool, futures + i, futures + i - 1, partial_multiplication);
        }
        
        long *result_p = (long *) await(futures + n - 1);
        long ret = *result_p;
        free(result_p);
        free(futures);
        return ret;
    }
}

int main() {
    thread_pool_t pool;
    thread_pool_init(&pool, 3);
    
    int n;
    scanf("%d", &n);
    
    long factorial = count_factorial(n, &pool);
    printf("%ld", factorial);
    
    thread_pool_destroy(&pool);
    return 0;
}
