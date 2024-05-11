// Group members:
//   Tran Tuan Kiet - 2252392
//   Ho Trong Nghia - 2252526
//   Nguyen Nghiem  - 2252521
//   Dinh Hai Nam   - 2252498

#ifndef FORKPOOL_H_
#define FORKPOOL_H_

#ifndef FORKPOOL_NUM_WORKERS
#define FORKPOOL_NUM_WORKERS 8
#endif

#ifndef FORKPOOL_SHARED_MEM_SIZE
#define FORKPOOL_SHARED_MEM_SIZE (sizeof(int) * FORKPOOL_NUM_WORKERS)
#endif

typedef void forkpool_task_fn_t(int, int *);

typedef struct {
    int shm_fd;
    int* task_out;
    pid_t pids[FORKPOOL_NUM_WORKERS];
} forkpool_t;

void forkpool_init(forkpool_t *);
void forkpool_fork(forkpool_t *, forkpool_task_fn_t *);
void forkpool_join(forkpool_t *);
void forkpool_free(forkpool_t *);

#endif
