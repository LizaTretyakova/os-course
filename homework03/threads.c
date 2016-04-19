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
volatile pid_t is_set_threading;

/*
void lock(lock_descriptor* ld) {
    __asm__("cli");

    printf("in lock\n");
    if(is_set_threading == FALSE) {
        printf("not set up\n");
        __asm__("sti");
        return;
    }

    barrier();
    while(ld->is_occupied) {
        barrier();
        schedule();
    }
    ld->is_occupied = TRUE;

    printf("Lock done\n");

    __asm__("sti");
}

void unlock(lock_descriptor* ld) {
    __asm__("cli");

    printf("in unlock\n");
    if(is_set_threading == FALSE) {
        printf("not set up\n");
        __asm__("sti");
        return;
    }

    barrier();
    ld->is_occupied = FALSE;
    barrier();
    printf("Unlock done\n");

    __asm__("sti");
}
//*/

uint64_t lock(lock_descriptor *ld) {
    uint64_t result = get_rflags();
    (void)ld;
    __asm__("cli");
    return result;
}

void unlock(lock_descriptor *ld, uint64_t rflags) {
    (void)ld;
    if (rflags & (1 << 9)) {
        __asm__("sti");
    }
}

void setup_threading() {
    cnt = 0;

    head = cur = kmem_alloc(sizeof(thread));
    common_ld = kmem_alloc(sizeof(lock_descriptor));
    common_ld->is_occupied = FALSE;
    list_init((struct list_head*)head);
    head->id = cnt;

    is_set_threading = TRUE;
}

thread* create_thread(void (*fptr)(void*), void* arg) {

    // get some memory for stack
    void* new_stack_mem = kmem_alloc(2 * PAGE_SIZE);
    // get memory for a new discriptor
    thread* new_thread = kmem_alloc(sizeof(thread));

    if (new_stack_mem == NULL || new_thread == NULL) {
        return NULL;
    }

/**/uint64_t flags = lock(common_ld);

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

/**/unlock(common_ld, flags);

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
        cur = (thread*)(((struct list_head*)cur)->next);
        barrier();
        switch_threads(&(((thread*)(((struct list_head*)cur)->prev))->rsp), cur->rsp);
        barrier();
        printf("switched threads from %llu to %llu\n", cur->id, ((thread*)(((struct list_head*)cur)->next))->id);
    }
}

void exit(thread* self) {
//**/uint64_t flags = lock(common_ld);
    self->is_dead = TRUE;
//**/unlock(common_ld, flags);
}

void join(thread* who) {
    while(who->is_dead == FALSE) {
        barrier();
    }
/**/uint64_t flags = lock(common_ld);
    list_del((struct list_head*)who);
/**/unlock(common_ld, flags);
    kmem_free(who->init_stack);
    kmem_free(who);
}

void thread_wrapper(thread* t) {
    printf("in thread_wrapper\n");
    __asm__("sti");
    (t->fptr)(t->arg);
    uint64_t flags = lock(common_ld);
    exit(t);
    unlock(common_ld, flags);
    schedule();
    printf("We shouldn't get here, but let call it while(true)\n");
    while(1);
}

uint64_t get_rflags() {
    uint64_t rflags;
    __asm__("pushfq\npopq %0" : "=g"(rflags));
    return rflags;
}

void set_rflags(uint64_t rflags) {
    __asm__("push %0\npopfq" :: "g"(rflags));
}
