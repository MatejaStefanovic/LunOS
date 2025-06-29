#ifndef __KERNEL_REGISTERS_H
#define __KERNEL_REGISTERS_H

#include <stdint.h>

struct interrupt_frame { 
    uint64_t cr2;
    uint64_t int_no;
    uint64_t err_code;
};

struct task_context {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip; // 120
    uint64_t cs;
    uint64_t rflags;
    uint64_t stack_ptr;
    uint64_t ss;  
};

#endif
