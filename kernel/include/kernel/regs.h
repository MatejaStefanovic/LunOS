#ifndef __KERNEL_REGISTERS_H
#define __KERNEL_REGISTERS_H

#include <stdint.h>

struct regs_t {
    uint32_t cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
    uint32_t int_no, err_code;                
};

#endif
