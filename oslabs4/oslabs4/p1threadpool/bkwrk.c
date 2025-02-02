// Group members:
//   Tran Tuan Kiet - 2252392
//   Ho Trong Nghia - 2252526
//   Nguyen Nghiem  - 2252521
//   Dinh Hai Nam   - 2252498

#define _GNU_SOURCE

#include "bktpool.h"
#include <signal.h>
#include <stdio.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

#define DEBUG
#define INFO
// #define WORK_THREAD

#ifndef WORK_THREAD
int pipe_fds[MAX_WORKER][2];

void init_pipes() {
    for (int i = 0; i < MAX_WORKER; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}

typedef struct {
  void *arg;
  int pipe_fd;
} worker_args_t;
#endif

int bkwrk_worker(void * arg) {
  sigset_t set;
  int sig;
  int s;
#ifdef WORK_THREAD
  int i = * ((int * ) arg); // Default arg is integer of workid
#else
  worker_args_t *worker_args = (worker_args_t *)arg;
  int i = *((int *)(worker_args->arg));
  int fd = worker_args->pipe_fd;
#endif
  struct bkworker_t * wrk = & worker[i];

#ifdef WORK_THREAD
  /* Taking the mask for waking up */
  sigemptyset( & set);
  sigaddset( & set, SIGUSR1);
  sigaddset( & set, SIGQUIT);
#endif

#ifdef DEBUG
  fprintf(stderr, "worker %i start living tid %d \n", i, getpid());
  fflush(stderr);
#endif

  while (1) {
#ifdef WORK_THREAD
    /* wait for signal */
    s = sigwait( & set, & sig);
    if (s != 0)
      continue;
#else
    char buffer[1];
    if (read(fd, buffer, 1) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
#endif

#ifdef INFO
    fprintf(stderr, "worker wake %d up\n", i);
    fflush(stderr);
#endif

    /* Busy running */
    if (wrk -> func != NULL)
      wrk -> func(wrk -> arg);

    /* Advertise I DONE WORKING */
    wrkid_busy[i] = 0;
    worker[i].func = NULL;
    worker[i].arg = NULL;
    worker[i].bktaskid = -1;
  }
}

int bktask_assign_worker(unsigned int bktaskid, unsigned int wrkid) {
  if (wrkid < 0 || wrkid > MAX_WORKER)
    return -1;

  struct bktask_t * tsk = bktask_get_byid(bktaskid);

  if (tsk == NULL)
    return -1;

  /* Advertise I AM WORKING */
  wrkid_busy[wrkid] = 1;

  worker[wrkid].func = tsk -> func;
  worker[wrkid].arg = tsk -> arg;
  worker[wrkid].bktaskid = bktaskid;

  printf("Assign tsk %d wrk %d \n", tsk -> bktaskid, wrkid);
  return 0;
}

int bkwrk_create_worker() {
#ifndef WORK_THREAD
  init_pipes();
#endif

  unsigned int i;

  for (i = 0; i < MAX_WORKER; i++) {
#ifdef WORK_THREAD
    void ** child_stack = (void ** ) malloc(STACK_SIZE);
    unsigned int wrkid = i;
    pthread_t threadid;

    sigset_t set;
    int s;

    sigemptyset( & set);
    sigaddset( & set, SIGQUIT);
    sigaddset( & set, SIGUSR1);
    sigprocmask(SIG_BLOCK, & set, NULL);

    /* Stack grow down - start at top*/
    void * stack_top = child_stack + STACK_SIZE;

    wrkid_tid[i] = clone( & bkwrk_worker, stack_top,
      CLONE_VM | CLONE_FILES,
      (void * ) & i);
#ifdef INFO
    fprintf(stderr, "bkwrk_create_worker got worker %u\n", wrkid_tid[i]);
#endif
    usleep(100);
#else
    /* TODO: Implement fork version of create worker */
    // Written by: Nguyen Nghiem
    pid_t pid;
    pid = fork();
    if (pid == 0) {
      close(pipe_fds[i][1]);
      worker_args_t x;
      x.arg = (void *)&i;
      x.pipe_fd = pipe_fds[i][0];
      bkwrk_worker((void *)&x);
      exit(0);
    } else if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else {
      wrkid_tid[i] = pid;
#ifdef INFO
      fprintf(stderr, "bkwrk_create_worker got worker %u\n", wrkid_tid[i]);
      fflush(stderr);
#endif
      usleep(100);
    }
#endif
  }

  return 0;
}

int bkwrk_get_worker() {
  /* TODO Implement the scheduler to select the resource entity 
   * The return value is the ID of the worker which is not currently 
   * busy or wrkid_busy[1] == 0 
   */
  // Written by: Tran Tuan Kiet
  unsigned int i = -1;
  for (i = 0; i < MAX_WORKER; i++) {
    if (wrkid_busy[i] == 0) {
      return i;
    }
  }

  if (i == -1) {
    // All workers busy! Find the earliest worker.
    unsigned int selected = 0;
    unsigned int best = wrkid_tid[0];
    for (i = 1; i < MAX_WORKER; i++) {
      if (wrkid_tid[i] < best) {
        best = wrkid_tid[i];
        selected = i;
      }
    }

#ifdef WORK_THREAD
    waitpid(wrkid_tid[selected], 0, 0);
    return selected;
#else
    int status;
    while (true) {
      waitpid(wrkid_tid[selected], &status, 0);
      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        break;
      }
    }

    return selected;
#endif
  }
}

int bkwrk_dispatch_worker(unsigned int wrkid) {

#ifdef WORK_THREAD
  unsigned int tid = wrkid_tid[wrkid];

  /* Invalid task */
  if (worker[wrkid].func == NULL)
    return -1;

#ifdef DEBUG
  fprintf(stderr, "brkwrk dispatch wrkid %d - send signal %u \n", wrkid, tid);
#endif

  syscall(SYS_tkill, tid, SIG_DISPATCH);
#else
  /* TODO: Implement fork version to signal worker process here */
  // Written by: Tran Tuan Kiet

  unsigned int pid = wrkid_tid[wrkid];
  if (worker[wrkid].func == NULL)
    return -1;

#ifdef DEBUG
  fprintf(stderr, "brkwrk dispatch wrkid %d - send signal %u \n", wrkid, pid);
  fflush(stderr);
#endif

  if (write(pipe_fds[wrkid][1], "D", 1) == -1) {
    perror("write");
    return -1;
  }
#endif
}