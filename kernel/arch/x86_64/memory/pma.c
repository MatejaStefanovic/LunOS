#include <kernel/pma.h>

struct buddy_arena buddy_arenas[MAX_BUDDY_ARENAS];

void parse_multiboot_mem_info(unsigned long addr){
    /* kernel is placed in higher half but grub places multiboot information 
     * struct at a low physical address so we need to adjust for out virtual 
     * address space mapping */
    addr+= KERNEL_VIRTUAL_BASE; 
    
    multiboot_info_t *mbi;
    mbi = (multiboot_info_t *) addr;

    if (CHECK_FLAG (mbi->flags, 0))
        kprintf ("mem_lower = %uKB, mem_upper = %uKB\n",
            (unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

    if (!CHECK_FLAG (mbi->flags, 6)){
        kprintf("CRITICAL ERROR: Couldn't get mmap from multiboot information struct\n");
        for(;;); // Gotta halt...
    }    
    
    unsigned long mmap_phys_start = mbi->mmap_addr;
    unsigned long mmap_phys_end = mbi->mmap_addr + mbi->mmap_length;
    unsigned long current_phys = mmap_phys_start;

    init_buddy_arenas(current_phys, mmap_phys_end);
}

void init_buddy_arenas(unsigned long current_phys, unsigned long mmap_phys_end){
    uint8_t buddy_arena_counter = 0;
    while (current_phys < mmap_phys_end) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)(current_phys + KERNEL_VIRTUAL_BASE);
       
        // If we somehow get addresses above 4GB we don't want those as we're dealing with 
        // a 32 bit system which means virutal address space is maximum 4GB 
        // we also don't want anything other than type 0x1 which means that the memory is free
        if((mmap->addr >> 32) != 0 || ((mmap->len >> 32) != 0) || mmap->type != 0x1){
            current_phys += mmap->size + sizeof(mmap->size);
            continue;
        }
        
        kprintf ("size = 0x%x, base_addr = 0x%x%x,"
            " length = 0x%x%x, type = 0x%x\n",
            (unsigned) mmap->size,
            (unsigned) (mmap->addr >> 32),
            (unsigned) (mmap->addr & 0xffffffff),
            (unsigned) (mmap->len >> 32),
            (unsigned) (mmap->len & 0xffffffff),
            (unsigned) mmap->type);
        

        uint32_t base = mmap->addr & 0xffffffff;
        uint32_t len = mmap->len & 0xffffffff;

        if(add_buddy_arena(buddy_arena_counter, base, len))
            ++buddy_arena_counter;
        
        current_phys += mmap->size + sizeof(mmap->size);
    }    
}

int add_buddy_arena(uint8_t buddy_arena_counter, uint32_t base, uint32_t len){
    // Align base up to the next page boundary
    // 0x1003 rounds up to 0x2000, if we used PAGE_FRAME_SIZE instead of
    // PAGE_FRAME_SIZE -1 we'd round down, this method rounds up
    // This is necessary for buddy allocation to work
    uint32_t aligned_base = (base + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
    uint32_t end = base + len;

    // If aligning base overshoots the region, there's nothing usable
    if (aligned_base >= end) return 0;

    uint32_t aligned_len = end - aligned_base;
    buddy_arenas[buddy_arena_counter].base = aligned_base;
    buddy_arenas[buddy_arena_counter].length = aligned_len;
    uint8_t max_order = 0;

    /* As block sizes are a power of 2 * 4096 we figure out what is the max power of 2
     * for our current buddy arena without overshooting
     * example: 3KB space - 1*4096 -> max order 0 but we overshot so we cannot allocate */
    while(((1U << (max_order + 1)) * PAGE_FRAME_SIZE) <= len) {
        ++max_order;
    }

    buddy_arenas[buddy_arena_counter].max_arena_order = max_order;
    
    // Initialize the free_list to NULL
    for (int i = 0; i <= MAX_SUPPORTED_ORDER; ++i) {
        buddy_arenas[buddy_arena_counter].free_list[i] = NULL;
    }
    
    init_arena_metadata(&buddy_arenas[buddy_arena_counter]);

    populate_buddy_blocks(buddy_arena_counter);
    
    return 1;
}


void populate_buddy_blocks(uint8_t buddy_arena_counter) {
    // I am too lazy to keep writing buddy_arenas[buddy_arena_counter]->something
    struct buddy_arena *arena = &buddy_arenas[buddy_arena_counter];

    uint32_t current_addr = arena->base + KERNEL_VIRTUAL_BASE;
    uint32_t remaining = arena->length;
    uint8_t max_order = arena->max_arena_order;

    while (remaining >= PAGE_FRAME_SIZE) {
        int order = max_order;

        // Find largest order block that fits and is aligned
        // This is important because when memory is not aligned 
        // BAD THINGS HAPPEN
        while (order > 0) {
            uint32_t block_size = (1U << order) * PAGE_FRAME_SIZE;
            if ((current_addr % block_size) == 0 && block_size <= remaining) {
                break;
            }
            --order;
        }

        uint32_t block_size = (1U << order) * PAGE_FRAME_SIZE;

        // Check metadata capacity
        if (arena->metadata_count >= arena->metadata_capacity) {
            // No more metadata blocks left to allocate, stop here
            break;
        }

        // Get next metadata block
        struct free_block *meta_block = &arena->metadata_blocks[arena->metadata_count++];

        // Initialize metadata for this free block
        meta_block->current_order = order;
        meta_block->phys_addr = current_addr - KERNEL_VIRTUAL_BASE;  // Store physical address
        meta_block->next = arena->free_list[order];

        // Insert metadata block into freelist
        arena->free_list[order] = meta_block;

        // Move to next block
        current_addr += block_size;
        remaining -= block_size;
    }
    print_buddy_arena(buddy_arena_counter);
}
void init_arena_metadata(struct buddy_arena *arena) {
    uint32_t max_blocks = arena->length / PAGE_FRAME_SIZE;
    uint32_t metadata_size = max_blocks * sizeof(struct free_block);
    uint32_t metadata_pages = (metadata_size + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;

    arena->metadata_capacity = max_blocks;
    arena->metadata_blocks = (struct free_block *)(arena->base + KERNEL_VIRTUAL_BASE);

    // Adjust arena base and length to exclude metadata space
    arena->base += metadata_pages * PAGE_FRAME_SIZE;
    arena->length -= metadata_pages * PAGE_FRAME_SIZE;

    arena->metadata_count = 0;
}

void print_buddy_arena(uint8_t buddy_arena_counter) {
    struct buddy_arena *arena = &buddy_arenas[buddy_arena_counter];

    kprintf("Buddy Arena %d freelists:\n", buddy_arena_counter);
    for (int order = 0; order <= arena->max_arena_order; ++order) {
        kprintf("Order %d: ", order);
        struct free_block *block = arena->free_list[order];
        int count = 0;
        while (block) {
            kprintf("[phys: 0x%lx] -> ", block->phys_addr);
            block = block->next;
            count++;
            if (count > 20) { // safety to avoid infinite loops
                kprintf("...");
                break;
            }
        }
        kprintf("NULL\n");
    }
}

