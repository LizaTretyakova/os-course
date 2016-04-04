#ifndef _THREADS_
#define _THREADS_

#include "list.h"
#include <sys/types.h>

#define TRUE  1
#define FALSE 0

#define NEW   0
#define OLD   1
#define DEAD  2

typedef struct lock_descriptor_t {
    int is_occupied;
} lock_descriptor;

typedef struct thread_t {
    struct list_head node;
    pid_t id;
    uint64_t is_dead;
    void* init_stack;
    void* rsp;

    void (*fptr)(void*);
    void* arg;
} thread;

/*
typedef struct wrapper_mix_t {
    thread* t;
} wrapper_mix;
*/

extern thread* cur;
extern thread* head; // idle thread
extern lock_descriptor* common_ld;
extern volatile pid_t cnt;

void setup_threading();
thread* create_thread(void (*fptr)(void*), void* arg);
void schedule();
void exit(thread*);
void join(thread*);


void lock(lock_descriptor*);
void unlock(lock_descriptor*);

uint64_t get_rflags();

void switch_threads(void** , void* );
void init_stack(void** , thread* , uint64_t);

#endif
