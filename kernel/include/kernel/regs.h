#ifndef __KERNEL_REGISTERS_H
#define __KERNEL_REGISTERS_H

#include <stdint.h>

struct regs {
    // Manually pushed registers
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    // CR2 for page faults
    uint64_t cr2;
    uint64_t int_no;
    uint64_t err_code;
    
    // CPU-pushed values
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t userrsp;
    uint64_t ss;
};

#endif
