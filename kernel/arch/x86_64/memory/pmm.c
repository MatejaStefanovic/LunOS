#include <kernel/pmm.h>

#define SLAB_THRESHOLD 2048  // Use slab for allocations <= 2KB

void *kmalloc(size_t size) {
    if (!size) {
        KWARN("Ayo why'd you request nothing?\n");
        return NULL;
    }
    
    // Use slab allocator for small allocations
    if (size <= SLAB_THRESHOLD) {
        void *ptr = slab_alloc_size(size);
        if (ptr) {
            return ptr;
        }
        // Fall back to buddy allocator if slab allocation fails
        KWARN("Slab allocation failed for size %lu, falling back to buddy\n", size);
    }
    
    // Use buddy allocator for large allocations or slab fallback
    // Account for header and end magic overhead
    size_t total_size = sizeof(struct alloc_header) + size + sizeof(uint32_t);
    uint8_t order = 0;
    size_t block_size = PAGE_FRAME_SIZE;
    
    while (block_size < total_size) {
        ++order;
        block_size <<= 1;
    }
    
    if (order >= MAX_SUPPORTED_ORDER) {
        KERROR("Not enough memory to allocate\n");
        return NULL;
    }
    
    uint64_t phys_addr = buddy_alloc_pages(order);
    if (phys_addr == 0) {
        KERROR("Buddy failed to allocate pages\n");
        return NULL;
    }
    
    void *virt_addr = phys_to_virt(phys_addr);
    struct alloc_header *alloc_h = (struct alloc_header *)virt_addr;
    alloc_h->magic = ALLOC_MAGIC;
    alloc_h->size = size;
    alloc_h->order = order;
   
    // Cast is for pointer arithmetic, C doesn't allow void * arithmetic 
    // start of data points to rigtt after our header because that's where our real
    // allocated memory starts, end magic is placed right where that memory ends
    char *start_of_data = (char *)virt_addr + sizeof(struct alloc_header);
    uint32_t *end_magic = (uint32_t *)(start_of_data + alloc_h->size);
    *end_magic = ALLOC_MAGIC;
    
    return (void *)start_of_data;
}

void kfree(void *ptr){
    if(!ptr){
        KERROR("Passed a NULL pointer to kfree - don't do this\n");
        return;
    }

    struct slab *slab = slab_find_containing(ptr);
    if (slab) {
        slab_free(ptr);
        return;
    }

    // We get our header pretty much the same way we skipped over it last time
    // our ptr points to right after the header so what we want is to just
    // subtract the start address of that with the size of our alloc_header struct
    // and that way we got our header back
    struct alloc_header* header = (struct alloc_header*)((char*)ptr - sizeof(struct alloc_header));

    // The magic number is used after we retrieve the allocated memory to check whether 
    // something messed with our values, if it is intact we're good, same as above
    if(header->magic != ALLOC_MAGIC){
        KERROR("Uh oh something ran over our allocated memory - this ain't good\n");
        
        // Remember we placed it ourselves right after kmalloc requested size
        // [header][requested size to alloc][end magic] is what it looks like in 
        // memory and this ptr starts here -^
        uint32_t *end_magic = (uint32_t*)((char*)ptr + header->size);
        if(*end_magic != ALLOC_MAGIC){
            KERROR("Uh oh not only has something ran over our allocated memory it went past it\n");
            return;
        }
        return;
    }

    uint64_t phys_addr = virt_to_phys(header);
    buddy_free_pages(phys_addr, header->order);
}

uint64_t pmm_alloc_page(){
    return buddy_alloc_page();
}

void pmm_free_page(uint64_t phys){
    return buddy_free_page(phys);
}
