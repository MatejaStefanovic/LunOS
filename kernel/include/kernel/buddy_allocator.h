#ifndef __KERNEL_BUDDY_ALLOCATOR_H
#define __KERNEL_BUDDY_ALLOCATOR_H

#define MAX_SUPPORTED_ORDER 20
#define MAX_BUDDY_ARENAS 32
#include <stdint.h> 
#include <stddef.h>
struct free_block {
    uint8_t current_order;
    uint32_t phys_addr;           // Physical address of the free block
    struct free_block *next;
};

struct buddy_arena{
    uint32_t base;              // Free memory starts at this address
    uint32_t length;            // Size in bytes
    uint8_t max_arena_order;    // Max power of 2 for block size 
    /* Array of pointers to free blocks differing in size by order of 2
     * freelist[19] is the biggest possible block which is 2GB and the 
     * lowest possible is freelist[0] which corresponds to a block size of 4096 bytes (1 page)
     * each free_block points to the next from highest to lowers 19->0 */
    struct free_block *free_list[MAX_SUPPORTED_ORDER + 1];

    /* Array of metadata blocks in mapped virtual memory 
     * without this we'd need to map phyiscal memory where the free
     * memory we got from grubs multiboot mmap started and repeat that
     * for each different memory region which is annoying and would require
     * us to map a mb or two at each base addr which is a bad idea */
    struct free_block *metadata_blocks;  
    uint32_t metadata_count;  
    uint32_t metadata_capacity;  
};

extern struct buddy_arena buddy_arenas[MAX_BUDDY_ARENAS];

#endif
