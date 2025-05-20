#ifndef __KERNEL_REGISTERS_H
#define __KERNEL_REGISTERS_H

struct regs_t {
    uint32_t gs, fs, es, ds;                  // Pushed manually
    uint32_t edi, esi, ebp, esp,              // Pushed by pushl
             ebx, edx, ecx, eax;
    uint32_t int_no, err_code;                // Pushed manually in stub
    uint32_t eip, cs, eflags, useresp, ss;    // Pushed automatically by the processor
};

#endif
