#include "process.h"
#include "memoryManager.h"
#include "lib.h"
#include <stdint.h>
#include <stddef.h>

#define STACK_SIZE 4096

static int next_pid = 1;

static int generate_pid(void) {
    return next_pid++;
}

Process *create_process(char *name, void (*function)(void *), void *arg, int priority) {
    Process *p = (Process *)mm_alloc(sizeof(Process));
    if (p == NULL)
        return NULL;

    p->pid = generate_pid();
    strncpy(p->name, name, sizeof(p->name));
    p->state = READY;
    p->priority = priority;
    p->foreground = 1;
    p->next = NULL;

    p->stack_base = mm_alloc(STACK_SIZE);
    if (p->stack_base == NULL) {
        mm_free(p);
        return NULL;
    }

    uint64_t *sp = (uint64_t *)((uint8_t *)p->stack_base + STACK_SIZE);
    uint64_t top = (uint64_t)sp;

    // iretq frame (CPU pops these: rip, cs, rflags, rsp, ss)
    *(--sp) = 0x0;                  // SS
    *(--sp) = top;                  // RSP
    *(--sp) = 0x202;                // RFLAGS: IF=1
    *(--sp) = 0x8;                  // CS: kernel code segment
    *(--sp) = (uint64_t)function;   // RIP

    // GPR frame (popState pops r15 first -> r15 must be at lowest address)
    *(--sp) = 0;                    // rax
    *(--sp) = 0;                    // rbx
    *(--sp) = 0;                    // rcx
    *(--sp) = 0;                    // rdx
    *(--sp) = 0;                    // rbp
    *(--sp) = (uint64_t)arg;        // rdi  <- first argument
    *(--sp) = 0;                    // rsi
    *(--sp) = 0;                    // r8
    *(--sp) = 0;                    // r9
    *(--sp) = 0;                    // r10
    *(--sp) = 0;                    // r11
    *(--sp) = 0;                    // r12
    *(--sp) = 0;                    // r13
    *(--sp) = 0;                    // r14
    *(--sp) = 0;                    // r15

    p->stack_pointer = sp;
    return p;
}