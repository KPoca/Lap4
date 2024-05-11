// Group members:
//   Tran Tuan Kiet - 2252392
//   Ho Trong Nghia - 2252526
//   Nguyen Nghiem  - 2252521
//   Dinh Hai Nam   - 2252498

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "forkpool.h"

void my_task(int task_id, int* out) {
    printf("Task %d launched.\n", task_id);
    int q = task_id;
    for (int i = 0; i < 10; i++) {
        q = (q % 4) + 3;
    }
    out[task_id] = q;
}

int main() {
    forkpool_t forkpool;
    forkpool_init(&forkpool);
    forkpool_fork(&forkpool, my_task);
    forkpool_join(&forkpool);

    int result = 0;
    for (int i = 0; i < FORKPOOL_NUM_WORKERS; i++) {
        result += forkpool.task_out[i];
    }
    printf("Output: %d\n", result);

    forkpool_free(&forkpool);

    return 0;
}
