#ifndef __KERNEL_REGISTERS_H
#define __KERNEL_REGISTERS_H

#include <stdint.h>

struct regs_t {// Pushed by pusha (order reversed to match stack layout)
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;

    // Manually pushed
    uint32_t ds;
    uint32_t cr2;
    uint32_t int_no;
    uint32_t err_code;

    // Pushed by CPU on any interrupt
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    // Only pushed if privilege level change (ring 3 → 0)
    uint32_t useresp;  // Only valid if from user mode
    uint32_t ss;       // Only valid if from user mode
};

#endif
