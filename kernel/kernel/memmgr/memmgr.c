#include <kernel/memmgr.h>
#include <kernel/pmm.h>

int mm_add_region(struct mem_descriptor_t *mm, vaddr_t start, 
        vaddr_t end, uint64_t flags){

    if(!mm || start >= end){
        KERROR("NULL task mem descriptor or start addr is bigger than end addr\n");
        return -1;
    }

    struct mem_region_t *region = kmalloc(sizeof(struct mem_region_t));

    region->start = start;
    region->end = end;
    region->flags = flags;
    region->next = mm->regions;
    mm->regions = region;

    return 0;
}

int mm_remove_region(struct mem_descriptor_t *mm, vaddr_t start, vaddr_t end){
    if(!mm || start >= end){
    KERROR("NULL task mem descriptor or start addr is bigger than end addr\n");
        return -1;
    }
    
    struct mem_region_t **current = &(mm->regions);
    while(*current){
        if((*current)->start == start && (*current)->end == end){
            struct mem_region_t *rmr = *current;
            *current = (*current)->next;
            kfree(rmr);
            return 0;
        }
        current = &((*current)->next);
    }
    KERROR("Couldn't find region to remove, check start and end VAs\n"); 
    return -1;
}

struct mem_region_t *mm_find_region(struct mem_descriptor_t *mm, vaddr_t vaddr){
    if(!mm){
        KERROR("Task mem descriptor provided is NULL\n");
        return NULL;
    }
    
    struct mem_region_t *region = mm->regions;
    while(region){
        if(region->start <= vaddr && vaddr <= region->end)
            return region;

        region = region->next;
    }
    KERROR("Couldn't find region");
    return NULL;
}

struct mem_descriptor_t *mm_alloc(){
    struct mem_descriptor_t *mem_desc = kmalloc(sizeof(struct mem_descriptor_t));
    struct addr_space_t *as = vmm_create_address_space();
    
    if(!as)
        return NULL;

    mem_desc->as = as;
    mem_desc->regions = NULL;
    mem_desc->brk = 0;
    mem_desc->mmap_base = 0;
    mem_desc->total_vm = 0;
    mem_desc->rss = 0;

    return mem_desc;
}

void mm_free(struct mem_descriptor_t *mm){
    if(!mm){
        KERROR("Cannot free NULL task memory descriptor\n");
        return;
    }

    vmm_destroy_address_space(mm->as);
    
    struct mem_region_t *current = mm->regions;
    struct mem_region_t *next;

    while (current) {
        next = current->next;
        kfree(current);
        current = next;
    }

    mm->regions = NULL; 
    kfree(mm);
}

int mm_setup_executable(struct mem_descriptor_t *mm, 
                       vaddr_t code_start, vaddr_t code_end, vaddr_t data_end) {
    mm_add_region(mm, code_start, code_end, RP_READ | RP_EXEC);
    
    vaddr_t data_start = (code_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    mm_add_region(mm, data_start, data_end, RP_READ | RP_WRITE);
    
    mm->brk = data_end + PAGE_SIZE;

    vaddr_t stack_top = STACK_TOP;
    vaddr_t stack_bottom = stack_top - STACK_SIZE + 1;
    vaddr_t guard_stack = stack_bottom - GUARD_SIZE;
    
    mm_add_region(mm, guard_stack, stack_bottom - 1, 0);
    mm_add_region(mm, stack_bottom, stack_top, RP_READ | RP_WRITE | RP_STACK);
    
    return 0;
}

bool mm_check_access(struct mem_descriptor_t *mm, vaddr_t addr, uint64_t access_flags) {
    struct mem_region_t *region = mm_find_region(mm, addr);
    if (!region) 
        return false; 
    
    // We hit a guard page which means something has overflown
    // This should lead to a SEGFAULT for when I implement syscalls and stuff
    if (region->flags == 0) 
        return false;
    
    // Must match access flags
    return (region->flags & access_flags) == access_flags;
}

int mm_expand_stack(struct mem_descriptor_t *mm, vaddr_t fault_addr) {
    paddr_t phys_page = pmm_alloc_page();
    if (phys_page == 0) {
        return -1; 
    }
    uint64_t flags = PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX;
    // vmm_map_page aligns both fault_addr and phys addr down to page size 
    // so it is perfect for allocating 1 page for the stack
    return vmm_map_page(mm->as, fault_addr, phys_page, flags);
}


int mm_expand_heap(struct mem_descriptor_t *mm, vaddr_t fault_addr) {
    vaddr_t grow_start = mm->brk;
    
    paddr_t phys_start = buddy_alloc_pages(HEAP_GROW_ORDER);
    if (phys_start == 0) {
        return -1;
    }
    
    uint64_t flags = PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX;
    int ret = vmm_map_range(mm->as, grow_start, phys_start, HEAP_GROW_SIZE, flags);

    if (ret != 0) 
        buddy_free_pages(phys_start, HEAP_GROW_ORDER);
    
    mm->brk = grow_start + HEAP_GROW_SIZE;
    
    return ret;
}

uint64_t err_code_to_access_flags(uint64_t error_code) {
    uint64_t access_flags = 0;
    
    // Instr means it is an execute access
    if (error_code & PF_INSTR) 
        access_flags |= RP_EXEC;
    // Write access
    else if (error_code & PF_WRITE)
        access_flags |= RP_WRITE;
    // Read access
    else 
        access_flags |= RP_READ;
    
    return access_flags;
}

void mm_page_fault_handler(uint64_t fault_addr, uint64_t error_code) {
    
    struct mem_descriptor_t *mm = NULL;
    uint64_t access_flags = err_code_to_access_flags(error_code); 
    if (!mm_check_access(mm, fault_addr, access_flags)) {
        // Should be SIGSEGV but we don't have that yet
        return;
    }

    struct mem_region_t *region = mm_find_region(mm, fault_addr);
    if (region->flags & RP_STACK) 
        mm_expand_stack(mm, fault_addr);
    else if (region->flags & RP_HEAP) 
        mm_expand_heap(mm, fault_addr);
    
}
