#include <kernel/buddy_allocator.h>

struct buddy_arena buddy_arenas[MAX_BUDDY_ARENAS];
uint8_t buddy_arena_counter = 0;

uint64_t hhdm_offset = 0;

void buddy_allocator_init(){
    struct limine_memmap_request *mmap_req = get_memmap_request();
   
    if(!mmap_req){
        kprintf("CRITICAL ERROR: Couldn't not get memory map\nHalting");
        for(;;);
    }
    struct limine_memmap_response *mmap_response = mmap_req->response;
    if(!mmap_response){
        kprintf("CRITICAL ERROR: Couldn't get response from mmap request\nHalting");
        for(;;);
    }
    
    for(uint64_t i = 0; i < mmap_response->entry_count; ++i){
        struct limine_memmap_entry *entry = mmap_response->entries[i];

        if(entry->type != 0)
            continue;
   
        if(buddy_arena_counter < MAX_BUDDY_ARENAS &&
                add_buddy_arena(buddy_arena_counter,entry->base, entry->length))
        {   
            
            uint64_t aligned_base = (entry->base + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
            uint64_t end = entry->base + entry->length;
            uint64_t aligned_len = (aligned_base >= end) ? 0 : (end - aligned_base);

            KSUCCESS("Buddy Arena %d initialized\n", buddy_arena_counter);
            kprintf("     Base:  0x%lx\n", aligned_base);
            kprintf("     Size:  %lu bytes (%lu MiB)\n", aligned_len, aligned_len / (1024 * 1024));
       
            print_arena_summary(buddy_arena_counter);
            ++buddy_arena_counter;        
        }
    }
}

int add_buddy_arena(uint8_t arena_idx, uint64_t base, uint64_t len){
    if (arena_idx >= MAX_BUDDY_ARENAS)
        return 0;    
    
    // Not aligning to page size is a B A D idea : ) 
    uint64_t aligned_base = (base + PAGE_FRAME_SIZE - 1) & ~(PAGE_FRAME_SIZE - 1);
    uint64_t end = base + len;
    
    // This means arena is too small
    if (aligned_base >= end) 
        return 0;
    
    uint64_t aligned_len = end - aligned_base;
    
    if (aligned_len < PAGE_FRAME_SIZE)
        return 0;
    
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
    return 1;
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
            
            if ((block_pfn & ((1ULL << order) - 1)) == 0) {
                break; // This alignment works
            }
            
            --order;
        }
        
        if (order < 0) break;
        
        uint64_t block_size = (1ULL << order) * PAGE_FRAME_SIZE;
        
        // With HHDM all of our memory is mapped to HHDM offset and above
        // and phys to virt will translate just translate with the offset in mind
        struct free_block *block = (struct free_block*)phys_to_virt(current_addr);
        
        block->current_order = order;
        block->phys_addr = current_addr;
        block->next = arena->free_list[order];
        
        arena->free_list[order] = block;
        
        current_addr += block_size;
        remaining -= block_size;
    }
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
