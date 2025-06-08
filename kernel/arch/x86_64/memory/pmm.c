#include <kernel/pmm.h>

void *kmalloc(size_t size){ 
    if(!size){
        KWARN("Ayo why'd you request nothing?\n");
        return NULL;
    }
                                                        // For our end magic
                                                        // it gives us overflow 
                                                        // detection
    size_t total_size = sizeof(struct heap_header) + size + sizeof(uint32_t);

    uint8_t order = 0;
    size_t block_size = PAGE_FRAME_SIZE;

    while(block_size < total_size){
        ++order;
        block_size <<= 1;
    }
    
    if(order >= MAX_SUPPORTED_ORDER){
        KERROR("Not enough memory to allocate\n");
        return NULL;
    }

    uint64_t phys_addr = buddy_alloc_pages(order);
    if(phys_addr == 0){
        KERROR("Buddy failed to allocate pages\n");
        return NULL;
    }

    void *virt_addr = phys_to_virt(phys_addr);

    struct heap_header* heap_h = (struct heap_header*)virt_addr;
    heap_h->magic = HEAP_MAGIC;
    heap_h->size = size;
    heap_h->order = order;
   
    // C doesn't allow void pointer arithmetic so we gotta cast to char *
    // essentially this is just a pointer that points to after our header
    // or rather it points to where usable memory starts
    char *start_of_data = (char*)virt_addr + sizeof(struct heap_header);

    // End magic is used to check for overflows, if something runs over
    // our allocated memory and keeps rampaging beyond our allocated block 
    // we can check it with this when freeing
    uint32_t *end_magic = (uint32_t*)(start_of_data + heap_h->size);
    *end_magic = HEAP_MAGIC;

    return (void*)start_of_data;
}

void kfree(void *ptr){
    if(!ptr){
        KERROR("Passed a NULL pointer to kfree - don't do this\n");
        return;
    }
    // We get our header pretty much the same way we skipped over it last time
    // our ptr points to right after the header so what we want is to just
    // subtract the start address of that with the size of our heap_header struct
    // and that way we got our header back
    struct heap_header* header = (struct heap_header*)((char*)ptr - sizeof(struct heap_header));

    // The magic number is used after we retrieve the heap to check whether 
    // something messed with our values, if it is intact we're good, same as above
    if(header->magic != HEAP_MAGIC){
        KERROR("Uh oh something ran over our heap - this ain't good\n");
        
        // Remember we placed it ourselves right after kmalloc requested size
        // [header][requested size to alloc][end magic] is what it looks like in 
        // memory and this ptr starts here -^
        uint32_t *end_magic = (uint32_t*)((char*)ptr + header->size);
        if(*end_magic != HEAP_MAGIC){
            KERROR("Uh oh not only has something ran over our heap it went past it\n");
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
