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

void forkpool_init(forkpool_t *f) {
    f->shm_fd = shm_open("/forkpool_shared", O_CREAT | O_RDWR, 0666);
    if (f->shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(f->shm_fd, FORKPOOL_SHARED_MEM_SIZE) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }
    f->task_out = mmap(NULL, FORKPOOL_SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, f->shm_fd, 0);
    if (f->task_out == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
}

void forkpool_fork(forkpool_t *f, forkpool_task_fn_t *task) {
    for (int i = 0; i < FORKPOOL_NUM_WORKERS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process (worker)
            task(i, f->task_out);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process (master)
            f->pids[i] = pid;
        }
    }
}

void forkpool_join(forkpool_t *f) {
    for (int i = 0; i < FORKPOOL_NUM_WORKERS; i++) {
        waitpid(f->pids[i], NULL, 0);
    }
}

void forkpool_free(forkpool_t *f) {
    if (munmap(f->task_out, FORKPOOL_SHARED_MEM_SIZE) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    // Close shared memory object
    if (close(f->shm_fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    // Unlink shared memory object
    if (shm_unlink("/forkpool_shared") == -1) {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }
}
