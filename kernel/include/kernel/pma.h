#ifndef __KERNEL_PHYSICAL_MEM_ALLOCATOR_H
#define __KERNEL_PHYSICAL_MEM_ALLOCATOR_H

#include <kernel/multiboot.h>
#include <kernel/klogging.h>
#include <kernel/buddy_allocator.h>

#define PAGE_FRAME_SIZE 4096

void parse_multiboot_mem_info(unsigned long meminfo_addr);
void init_buddy_arenas(unsigned long current_phys, unsigned long mmap_phys_end);
int add_buddy_arena(uint8_t ba_cnt,uint32_t base, uint32_t len);
void populate_buddy_blocks(uint8_t buddy_arena_counter);
void init_arena_metadata(struct buddy_arena *arena);
void print_buddy_arena(uint8_t buddy_arena_counter);

#endif
