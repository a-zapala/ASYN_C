#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>

#include "threadpool.h"
#include "future.h"

void *parital_multiplication(void *args, size_t argsz __attribute__((unused)), size_t *res_size) {
    int *value = args;
    int *sleep_time = (int *) args + 1;
    
    usleep(*sleep_time * 1000);
    
    *res_size = sizeof(int);
    int *ret = malloc(sizeof(int));
    *ret = *value;
    
    free(args);
    return ret;
}

int main() {
    thread_pool_t pool;
    thread_pool_init(&pool, 4);
    
    int w, k;
    scanf("%d\n%d", &w, &k);
    
    future_t *futures = malloc(sizeof(future_t) * w * k);
    
    for (int i = 0; i < w * k; i++) {
        int *args = malloc(sizeof(int) * 2);
        scanf("%d %d", args, args + 1);
        async(&pool, futures + i,
              (callable_t) {.function = parital_multiplication, .arg = args, .argsz = sizeof(int) *
                                                                                      2});
    }
    
    for(int i = 0 ; i < w; i++) {
        int sum = 0;
        for(int j = 0; j < k; j++) {
            int *res = (int *) await(futures + i*k + j);
            sum += *res;
            free(res);
        }
        printf("%d\n",sum);
    }
    
    free(futures);
    thread_pool_destroy(&pool);
    return 0;
}
