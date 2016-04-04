#include "threads.h"
#include "memory.h"
#include "kmem_cache.h"
#include "list.h"
#include "stdio.h"

#include <stddef.h>

#define RFLAGS_MASK 0xF12A

thread* cur;
thread* head;
lock_descriptor* common_ld;
volatile pid_t cnt;


void lock(lock_descriptor* ld) {
    __asm__("cli");

    while(ld->is_occupied) {
        schedule();
    }
    ld->is_occupied = TRUE;

    printf("Lock done\n");

    __asm__("sti");
}

void unlock(lock_descriptor* ld) {
    __asm__("cli");

    ld->is_occupied = FALSE;
    printf("Unlock done\n");

    __asm__("sti");
}

void setup_threading() {
    cnt = 0;

    head = cur = kmem_alloc(sizeof(thread));
    common_ld = kmem_alloc(sizeof(lock_descriptor));
    common_ld->is_occupied = FALSE;
    list_init((struct list_head*)head);
    head->id = cnt;
}

thread* create_thread(void (*fptr)(void*), void* arg) {
/**/lock(common_ld);

    // get some memory for stack
    void* new_stack_mem = kmem_alloc(2 * PAGE_SIZE);
    // get memory for a new discriptor
    thread* new_thread = kmem_alloc(sizeof(thread));

    if (new_stack_mem == NULL || new_thread == NULL) {
        return NULL;
    }

    list_add((struct list_head*)new_thread, (struct list_head*)head);
    new_thread->id = cnt;
    new_thread->is_dead = FALSE;
    new_thread->init_stack = new_stack_mem;
    new_thread->rsp = (char*)new_stack_mem + 2 * PAGE_SIZE;// - 1;
    new_thread->fptr = fptr;
    new_thread->arg = arg;

    uint64_t rflags = get_rflags();
    rflags &= RFLAGS_MASK;

    printf("Before init\n");
    init_stack(&(new_thread->rsp), new_thread, rflags);

    for (uint64_t* ptr = new_thread->rsp; (uint64_t)ptr < (uint64_t)new_stack_mem + 2 * PAGE_SIZE; ptr++) {
        printf("! new created stack: at %p lies 0x%lX.\n", ptr, *ptr);
    }


    printf("Initialized stack\n");

    ++cnt;
//    pid_t result = cnt - 1;

/**/unlock(common_ld);

    printf("created thread\n");

    return new_thread;
}

void schedule() {
    if (cur == NULL) {
        while(1);
    }
    while (((thread*)(((struct list_head*)cur)->next))->is_dead == TRUE
           && ((struct list_head*)cur)->next != (struct list_head*)cur) {
        cur = (thread*)(((struct list_head*)cur)->next);
    }
    if (((struct list_head*)cur)->next != (struct list_head*)cur) {
        printf("before switch cur=%p\n", cur);
        printf("!! cur->rsp at %p, new rsp is %p.\n", &(cur->rsp), ((thread*)(((struct list_head*)cur)->next))->rsp);
        switch_threads(&(cur->rsp), ((thread*)(((struct list_head*)cur)->next))->rsp);
        printf("switched threads from %llu to %llu\n", cur->id, ((thread*)(((struct list_head*)cur)->next))->id);
        cur = (thread*)(((struct list_head*)cur)->next);
    }
}

void exit(thread* self) {
/**/lock(common_ld);
    self->is_dead = TRUE;
/**/unlock(common_ld);
    while(1);
}

void join(thread* who) {
    while(who->is_dead == FALSE) {
        continue;
    }
/**/lock(common_ld);
    list_del((struct list_head*)who);
    kmem_free(who->init_stack);
    kmem_free(who);
/**/unlock(common_ld);
}

void thread_wrapper(thread* t) {
    printf("in thread_wrapper\n");
    (t->fptr)(t->arg);
    lock(common_ld);
    t->is_dead = TRUE;
    unlock(common_ld);
    schedule();
    printf("We shouldn't get here, but let call it while(true)\n");
    while(1);
}

uint64_t get_rflags() {
    uint64_t rflags;
    __asm__("pushfq\npopq %0" : "=g"(rflags));
    return rflags;
}
