#ifndef __KERNEL_BUDDY_ALLOCATOR_H
#define __KERNEL_BUDDY_ALLOCATOR_H

#define MAX_SUPPORTED_ORDER 20
#define MAX_BUDDY_ARENAS 32
#define PAGE_FRAME_SIZE 4096

#include <kernel/memutils.h>
#include <kernel/klogging.h>
struct free_block {
    uint8_t current_order;
    uint64_t phys_addr;         // Physical address of the free block
    struct free_block *next;
};

struct buddy_arena{
    uint64_t base;              // Free memory starts at this address
    uint64_t length;            // Size in bytes
    uint8_t max_arena_order;    // Max power of 2 for block size 
    /* Array of pointers to free blocks differing in size by order of 2
     * freelist[20] is the biggest possible block which is 4GB and the 
     * lowest possible is freelist[0] which corresponds to a block size of 
     * 4096 bytes  (1 page)
     * each free_block points to the next from highest to lowest 20->0 */
    struct free_block *free_list[MAX_SUPPORTED_ORDER + 1];
};

extern struct buddy_arena buddy_arenas[MAX_BUDDY_ARENAS];

void buddy_allocator_init(void);
int add_buddy_arena(uint8_t ba_cnt,uint64_t base, uint64_t len);
void populate_buddy_blocks(uint8_t buddy_arena_counter);
uint64_t buddy_alloc_pages(uint8_t order); 
uint64_t buddy_alloc_page(void);
void buddy_free_pages(uint64_t phys_addr, uint8_t order);
void buddy_free_page(uint64_t phys_addr);

// Debug functions
void print_buddy_arena(uint8_t buddy_arena_counter);
void print_arena_summary(uint8_t arena_idx);


static inline void* phys_to_virt(uint64_t phys_addr) {
    return (void*)(phys_addr + get_hhdm_offset());
}

static inline uint64_t virt_to_phys(void* virt_addr) {
    return (uint64_t)virt_addr - get_hhdm_offset();
}
#endif

