#ifndef __KERNEL_IDT_H
#define __KERNEL_IDT_H

#include <stdint.h>

#define IDT_SIZE 256

/* isr_t is a function pointer type used for
 * defining interrupt service routines
 * each function that is assigned to an isr_t will
 * handle a specific interrupt which means the type itself
 * is used to manage interrupt handlers*/
typedef void (*isr_t)();  

struct idt_entry{
    uint16_t offset_lower;
    uint16_t seg_selector;
    uint8_t  zero; // always 0, reserved by intel
    uint8_t  flags; // P DPL D TYPE
    uint16_t offset_higher;
}__attribute__((packed));

struct idt_ptr{
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

void setIdt(uint16_t, uint32_t);
void create_gate_entry(uint8_t, isr_t, uint16_t, uint8_t);
void init_idt(void);

#endif
