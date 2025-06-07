#include <kernel/buddy_allocator.h>

struct buddy_arena buddy_arenas[MAX_BUDDY_ARENAS];
uint8_t buddy_arena_counter = 0;

void buddy_allocator_init(){
    struct limine_memmap_request *mmap_req = get_memmap_request();
   
    if(!mmap_req){
        KERROR("CRITICAL ERROR: Couldn't not get memory map\nHalting");
        hcf();
    }
    struct limine_memmap_response *mmap_response = mmap_req->response;
    if(!mmap_response){
        KERROR("CRITICAL ERROR: Couldn't get response from mmap request\nHalting");
        hcf();
    }
    
    for(uint64_t i = 0; i < mmap_response->entry_count; ++i){
        struct limine_memmap_entry *entry = mmap_response->entries[i];

        if(entry->type != 0)
            continue;
        
        if(buddy_arena_counter >= MAX_BUDDY_ARENAS){
            KWARN("Number of arenas is too small, yell at the dev to increase it\n");
            break;
        }
   
        if(add_buddy_arena(buddy_arena_counter,entry->base, entry->length) == 0)
        {   
            uint64_t aligned_base = (entry->base + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
            uint64_t end = entry->base + entry->length;
            uint64_t aligned_len = (aligned_base >= end) ? 0 : (end - aligned_base);

            KSUCCESS("Buddy Arena %d initialized\n", buddy_arena_counter);
            kprintf("     Base:  0x%lx\n", aligned_base);
            kprintf("     Size:  %lu bytes (%lu MiB)\n", aligned_len, aligned_len / (1024 * 1024));
           
            ++buddy_arena_counter;        
        }
    }
}

int add_buddy_arena(uint8_t arena_idx, uint64_t base, uint64_t len){
    if (arena_idx >= MAX_BUDDY_ARENAS){
        KERROR("Not enough arenas, yell at the dev to increase it\n");
        return -1;    
    }
    // Not aligning to page size is a B A D idea : ) 
    uint64_t aligned_base = (base + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
    uint64_t end = base + len;
    
    // This means arena is too small
    if (aligned_base >= end) 
        return -1;
    
    uint64_t aligned_len = end - aligned_base;
    
    // This also means arena is too small
    if (aligned_len < PAGE_FRAME_SIZE)
        return -1;
    
    buddy_arenas[arena_idx].base = aligned_base;
    buddy_arenas[arena_idx].length = aligned_len;
    
    uint8_t max_order = 0;
    while (((1ULL << (max_order + 1)) * PAGE_FRAME_SIZE) <= aligned_len) {
        ++max_order;
    }
    
    if (max_order > MAX_SUPPORTED_ORDER) {
        max_order = MAX_SUPPORTED_ORDER;
    }
    
    buddy_arenas[arena_idx].max_arena_order = max_order;
    
    for (int i = 0; i <= MAX_SUPPORTED_ORDER; ++i) {
        buddy_arenas[arena_idx].free_list[i] = NULL;
    }
    
    populate_buddy_blocks(arena_idx);
    return 0;
}

void populate_buddy_blocks(uint8_t arena_idx){
    struct buddy_arena *arena = &buddy_arenas[arena_idx];
    
    uint64_t current_addr = arena->base;
    uint64_t remaining = arena->length;
    uint8_t max_order = arena->max_arena_order;
    
    while (remaining >= PAGE_FRAME_SIZE) {
        int order = max_order;
        
        // Find largest order block that fits and has proper buddy alignment
        while (order >= 0) {
            uint64_t block_size = (1ULL << order) * PAGE_FRAME_SIZE;
            
            if (block_size > remaining) {
                --order;
                continue;
            }
            
            // Check buddy alignment
            uint64_t block_offset = current_addr - arena->base;
            uint64_t block_pfn = block_offset / PAGE_FRAME_SIZE;
            
            if ((block_pfn & ((1ULL << order) - 1)) == 0) 
                break; // This alignment works
            
            --order;
        }
        
        if (order < 0) 
            break;
        
        uint64_t block_size = (1ULL << order) * PAGE_FRAME_SIZE;
        
        // With HHDM all of our memory is mapped to HHDM offset and above
        // and phys to virt will translate with the offset in mind
        struct free_block *block = (struct free_block*)phys_to_virt(current_addr);
        
        block->current_order = order;
        block->phys_addr = current_addr;
        block->next = arena->free_list[order];
        
        arena->free_list[order] = block;

        current_addr += block_size;
        remaining -= block_size;
    }
}

uint64_t buddy_alloc_pages(uint8_t order){
    if (order > MAX_SUPPORTED_ORDER)
        return 0;

    for(int i = 0; i < buddy_arena_counter; ++i){
        struct buddy_arena *arena = &buddy_arenas[i];

        // If we got a block of that order allocate it
        // Otherwise we deal with splitting...
        if(arena->free_list[order] != NULL){
            // Get the block and unlink it
            struct free_block *block = arena->free_list[order];
            arena->free_list[order] = block->next;
            return block->phys_addr;
        }

        // Start with one order higher and if that exists split it into 2
        // if not we go even higher to split that one and then we allocate 
        // the appropriate one
        for(int j = order + 1; j <= arena->max_arena_order; ++j){
            if(arena->free_list[j] == NULL)
                continue;
    
            // Unlink the block as we split it
            struct free_block *block = arena->free_list[j];
            arena->free_list[j] = block->next;
            
            // K = j - 1 as we try to get the appropriate order
            // This handles the case if we went up 2 orders higher (or more) 
            // instead of 1 - basically it just keeps on splitting until our
            // asked order
            uint64_t addr = block->phys_addr;
            for(int k = j - 1; k >= order; --k){
                // addr is the start of left buddy, we keep right buddy
                // and continue splitting the left
                uint64_t buddy_size = (1ULL << k) * PAGE_FRAME_SIZE;
                uint64_t buddy_addr = addr + buddy_size; 
                
                struct free_block *buddy = (struct free_block*)phys_to_virt(buddy_addr);
                buddy->current_order = k;
                buddy->phys_addr = buddy_addr;
                buddy->next = arena->free_list[k];
                arena->free_list[k] = buddy;
            }
            // left budy is now appropriate order and we 
            // return its address 
            return addr;
        } 
    }
    return 0;
}

void buddy_free_pages(uint64_t phys_addr, uint8_t order){
    // This should never happen ** I HOPE **
    if(order > MAX_SUPPORTED_ORDER){
        KERROR("Tried to free more memory than there is in the system!?\n");
        return;
    }

    // Find arena based on phys_addr
    struct buddy_arena *arena = NULL;
    for(int i = 0; i < buddy_arena_counter; ++i){
        if(phys_addr >= buddy_arenas[i].base &&
                phys_addr < buddy_arenas[i].base + buddy_arenas[i].length){

            arena = &buddy_arenas[i];
            break;
        }
    }
    
    // wtf
    if(arena == NULL){
        KERROR("Couldn't find arena\nAborting...\n");
        return;
    }
    // Time to coalsce

    kprintf("\nphys_addr: %lx\n", phys_addr);
    while(order < arena->max_arena_order){

        uint64_t block_size = (1ULL << order) * PAGE_FRAME_SIZE;
        // Buddies can be found by just xoring one of the buddy's 
        // physical address and the block size of that buddy's order
        // Thank you Donald Knuth 
        uint64_t buddy_addr = phys_addr ^ block_size; 
        
        kprintf("buddy_addr %lx\n", buddy_addr);
        
        struct free_block **current = &arena->free_list[order];
        bool found_buddy = false;
        
        while(*current){
            // If we found our buddy then that is great, it means
            // we can merge into a bigger block but first we need to unlink 
            // our buddy 
            if((*current)->phys_addr == buddy_addr){
                struct free_block *buddy = *current;
                *current = buddy->next;
                found_buddy = true;
                break;
            }
            current = &(*current)->next; 
        }
        
        if(!found_buddy)
            break;
        // We wanna use the lower address always for our new 
        // bigger block
        if(buddy_addr < phys_addr)
            phys_addr = buddy_addr;
        
        ++order;
    }
    // Link the new buddy
    // Note: we don't do this inside the while loop as we might also find
    // the buddy of our new bigger block which can also be merge, hence we only
    // link the biggest possible block (if we don't find the buddy we break out
    // of the while loop and get here)
    struct free_block *block = (struct free_block*)phys_to_virt(phys_addr);
    block->current_order = order;
    block->phys_addr = phys_addr;
    block->next = arena->free_list[order];
    arena->free_list[order] = block;
}

uint64_t buddy_alloc_page(void) {
    return buddy_alloc_pages(0);
}

void buddy_free_page(uint64_t phys_addr) {
    buddy_free_pages(phys_addr, 0);
}









void print_buddy_arena(uint8_t buddy_arena_counter) {
    struct buddy_arena *arena = &buddy_arenas[buddy_arena_counter];
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

void print_arena_summary(uint8_t arena_idx) {
    struct buddy_arena *arena = &buddy_arenas[arena_idx];
    uint64_t total_free_bytes = 0;

    kprintf("\n[+] Arena %d Free Summary:\n", arena_idx);

    for (int order = 0; order <= arena->max_arena_order; ++order) {
        struct free_block *block = arena->free_list[order];
        uint64_t block_count = 0;

        while (block) {
            ++block_count;
            block = block->next;
        }

        uint64_t block_size = (1ULL << order) * PAGE_FRAME_SIZE;
        uint64_t order_total = block_count * block_size;
        total_free_bytes += order_total;

        kprintf("     Order %d: %lu blocks x %lu bytes = %lu bytes\n",
                order, block_count, block_size, order_total);
    }

    kprintf("     -----------------------------------------------\n");
    kprintf("     Total free memory: %lu bytes (%lu KiB) and (%lu MiB)\n", 
            total_free_bytes, total_free_bytes / 1024, total_free_bytes / (1024*1024));
    kprintf("     Arena length:      %lu bytes (%lu KiB) and (%lu MiB)\n", 
            arena->length, arena->length / 1024, total_free_bytes / (1024*1024));

    uint64_t lost = arena->length - total_free_bytes;
    kprintf("     Difference (rounding/fragmentation): %lu bytes\n\n", lost);
}
