#ifndef __KERNEL_GDT_H
#define __KERNEL_GDT_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/compiler.h>

#define NUM_OF_ENTRIES 6

// Standard GDT entry (8 bytes)
struct gdt_entry {
    uint16_t limit_lower;
    uint16_t base_lower;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_higher;    
} _packed;

struct tss_descriptor {
    uint16_t limit_lower;
    uint16_t base_lower;
    uint8_t base_middle;
    uint8_t access;       
    uint8_t granularity;
    uint8_t base_high;
    uint32_t base_upper;  // Upper 32 bits of base (64-bit only)
    uint32_t reserved;    // Must be zero
} _packed;

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} _packed;

extern struct gdt_ptr gdtr;

struct task_state_seg {
    uint32_t reserved1;
    uint64_t rsp0;          // ring 0 stack ptr
    uint64_t rsp1;          // ring 1 stack ptr
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist1;          // Interrupt Stack Table entries
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;  // I/O map base address
} _packed;

struct global_descr_table {
    struct gdt_entry entries[5];
    struct tss_descriptor tss_desc;
} _packed;

extern void reload_gdt(void);
void init_gdt(void);

#endif
