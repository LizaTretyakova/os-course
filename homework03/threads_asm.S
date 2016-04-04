    .code64
    .global switch_threads
    .global init_stack
    .extern thread_wrapper

//; void switch_threads(void∗∗ old_sp, void∗ new_sp); // C signature
switch_threads: //; save volatile registers
    pushfq
    pushq %rdi //; just a trash, but let it be here to make the following call of popq %rdi below valid
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15


    movq %rsp, (%rdi) //; save SP
    movq %rsi, %rsp   //; restore SP

    popq %r15 //; restore volatile reigster
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    popq %rdi
    popfq

    ret

//; void* init_stack(void** new_sp, thread* t, uint64_t rflags);
init_stack:
    movq %rsp, %rax   //; save %rsp to recover it in the end
    movq (%rdi), %rsp //; set stack pointer to the start

    pushq $thread_wrapper  //; wrapper pointer
    pushq %rdx //; set rflags
    pushq %rsi //; thread pointer
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, (%rdi) //; set new_sp to the actual value
    movq %rax, %rsp   //; recover actual value of %rsp

    ret
