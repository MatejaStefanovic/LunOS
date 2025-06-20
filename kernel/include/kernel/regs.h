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

    // RSP MUST BE LAST!!
    // We use this struct for saving the state when an interrupt happens
    // however we never actually push RSP in our isr_stub and that's on purpose
    // this will only be used manually when creating tasks or context switching
    // it is fine to leave it uninitialized as userrsp and ss are uninitialized as well
    // whenever an interrupt happens in ring 0 as those are only pushed when we're in ring 3
    uint64_t rsp;
};

#endif
