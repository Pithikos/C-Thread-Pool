#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "thpool_old.h"

void sleep_task(void *boop){
    (void)boop;
    sleep(0.1);
}

int main(int argc, char **argv){
    char* p;
    if (argc != 3){
        printf("This benchmark needs exactly two arguments: thread number and task number.\n");
        return(1);
    }
    size_t num_threads = strtol(argv[1], &p, 10);
    size_t num_tasks   = strtol(argv[2], &p, 10);

    threadpool pool = thpool_init(num_threads);
    size_t n;
    for (n = 0; n < num_tasks; n++){
        thpool_add_work(pool, sleep_task, NULL);
    }
    thpool_wait(pool);
    thpool_destroy(pool);
}
