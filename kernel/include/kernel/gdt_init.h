#ifndef __KERNEL_GDT_H
#define __KERNEL_GDT_H

#include <stdint.h>

#define NUM_OF_ENTRIES 5

#define GDT_ENTRY_NULL      0
#define GDT_ENTRY_KCODE     1
#define GDT_ENTRY_KDATA     2
#define GDT_ENTRY_UCODE     3
#define GDT_ENTRY_UDATA     4
#define GDT_ENTRY_TSS       5

// Reminder, intel manual has GDT segement descriptor structure at 
// vol 3 ch 3 or refer to osdev GDT page
struct gdt_entry{
    uint16_t limit_lower;
    uint16_t base_lower;
    uint8_t base_middle;
    uint8_t access; // Access bits
    uint8_t granularity; // granularity as well as 4 bits for seg limit to total 20
    uint8_t base_high;    
} __attribute__((packed));


struct gdt_ptr{
    uint16_t limit; // size of GDT - 1
    uint32_t base;  // address of the first entry
} __attribute__((packed));

extern void setGdt(uint16_t, uint32_t);
extern void reloadSegments(void);

void set_gdt_entry(int, uint32_t, uint32_t, uint8_t, uint8_t);
void init_gdt(void);

#endif
