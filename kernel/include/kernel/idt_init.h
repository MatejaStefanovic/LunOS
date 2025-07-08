#ifndef __KERNEL_IDT_H
#define __KERNEL_IDT_H

#include <stdint.h>
#include <kernel/compiler.h>

#define IDT_SIZE 256

/* isr_t is a function pointer type used for
 * defining interrupt service routines
 * each function that is assigned to an isr_t will
 * handle a specific interrupt which means the type itself
 * is used to manage interrupt handlers*/
typedef void (*isr_t)();  

struct idt_entry_t {
	uint16_t    isr_low;    // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;  // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    ist;        // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes; // Type and attributes; see the IDT page
	uint16_t    isr_mid;    // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t    isr_high;   // The higher 32 bits of the ISR's address
	uint32_t    reserved;   // Set to zero
} _packed;

struct idt_ptr {
	uint16_t	limit;
	uint64_t	base;
} _packed;



extern void setIdt(uint16_t limit, uint64_t base);
void create_gate_entry(uint8_t entry_index, isr_t handler, uint16_t seg_selector, uint8_t flags);
void init_idt(void);
void reload_idt(void);

#endif
