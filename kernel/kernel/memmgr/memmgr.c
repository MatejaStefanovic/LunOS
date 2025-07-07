#include <kernel/memmgr.h>
#include <kernel/pmm.h>

int mm_add_region(struct mem_descriptor *mm, virt_addr start, 
        virt_addr end, uint64_t flags){

    if(!mm || start >= end){
        KERROR("NULL task mem descriptor or start addr is bigger than end addr\n");
        return -1;
    }

    struct mem_region *region = kmalloc(sizeof(*region));

    region->start = start;
    region->end = end;
    region->flags = flags;
    region->next = mm->regions;
    mm->regions = region;

    return 0;
}

int mm_remove_region(struct mem_descriptor *mm, virt_addr start, virt_addr end){
    if(!mm || start >= end){
    KERROR("NULL task mem descriptor or start addr is bigger than end addr\n");
        return -1;
    }
    
    struct mem_region **current = &(mm->regions);
    while(*current){
        if((*current)->start == start && (*current)->end == end){
            struct mem_region *rmr = *current;
            *current = (*current)->next;
            kfree(rmr);
            return 0;
        }
        current = &((*current)->next);
    }
    KERROR("Couldn't find region to remove, check start and end VAs\n"); 
    return -1;
}

struct mem_region *mm_find_region(struct mem_descriptor *mm, virt_addr vaddr){
    if(!mm){
        KERROR("Task mem descriptor provided is NULL\n");
        return NULL;
    }
    
    struct mem_region *region = mm->regions;
    while(region){
        if(region->start <= vaddr && vaddr <= region->end)
            return region;

        region = region->next;
    }
    KERROR("Couldn't find region");
    return NULL;
}

// Only for user space as kernel tasks will have NULL mem descriptor
struct mem_descriptor *mm_alloc(void){
    struct mem_descriptor *mem_desc = kmalloc(sizeof(*mem_desc));
    struct addr_space *as = vmm_create_address_space();
    
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

void mm_free(struct mem_descriptor *mm){
    if(!mm){
        KERROR("Cannot free NULL task memory descriptor\n");
        return;
    }

    vmm_destroy_address_space(mm->as);
    
    struct mem_region *current = mm->regions;
    struct mem_region *next;

    while (current) {
        next = current->next;
        kfree(current);
        current = next;
    }

    mm->regions = NULL; 
    kfree(mm);
}

int mm_setup_executable(struct mem_descriptor *mm, 
                       virt_addr code_start, virt_addr code_end, virt_addr data_end) {
    
    // TODO: virtually map code and data segment, we'll get some info from 
    // elf loader when I do make it
    mm_add_region(mm, code_start, code_end, RP_READ | RP_EXEC);
    virt_addr data_start = (code_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    mm_add_region(mm, data_start, data_end, RP_READ | RP_WRITE);

    virt_addr heap_start = (data_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    virt_addr heap_end = heap_start + HEAP_SIZE - 1;
    mm->brk = heap_start;
    
    virt_addr guard_heap = heap_end + GUARD_SIZE;
    mm_add_region(mm, heap_start, heap_end, RP_READ | RP_WRITE | RP_HEAP);  
    mm_add_region(mm, heap_end + 1, guard_heap, 0);

    virt_addr stack_top = STACK_TOP;
    virt_addr stack_bottom = stack_top - STACK_SIZE + 1;
    virt_addr guard_stack = stack_bottom - GUARD_SIZE;
    
    mm_add_region(mm, guard_stack, stack_bottom - 1, 0);
    mm_add_region(mm, stack_bottom, stack_top, RP_READ | RP_WRITE | RP_STACK);
    
    return 0;
}

bool mm_check_access(struct mem_descriptor *mm, virt_addr addr, uint64_t access_flags) {
    struct mem_region *region = mm_find_region(mm, addr);
    if (!region) 
        return false; 
    
    // We hit a guard page which means something has overflown
    // This should lead to a SEGFAULT for when I implement syscalls and stuff
    if (region->flags == 0) 
        return false;
    
    // Must match access flags
    return (region->flags & access_flags) == access_flags;
}

int mm_expand_stack(struct mem_descriptor *mm, virt_addr fault_addr) {
    phys_addr phys_page = pmm_alloc_page();
    if (phys_page == 0) {
        return -1; 
    }
    uint64_t flags = PTE_PRESENT | PTE_WRITABLE | PTE_USER | PTE_NX;
    // vmm_map_page aligns both fault_addr and phys addr down to page size 
    // so it is perfect for allocating 1 page for the stack
    return vmm_map_page(mm->as, fault_addr, phys_page, flags);
}


int mm_expand_heap(struct mem_descriptor *mm, virt_addr fault_addr) {
    if(fault_addr < mm->brk){
        KERROR("Fault address is lower than heap brk - something is seriously wrong\n");
        kprintf("Fault addr: %lx\nHeap brk: %lx\n", fault_addr, mm->brk);
        return 0;
    }

    virt_addr grow_start = mm->brk;
    
    phys_addr phys_start = buddy_alloc_pages(HEAP_GROW_ORDER);
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

static uint64_t err_code_to_access_flags(uint64_t error_code) {
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
    
    struct mem_descriptor *mm = NULL;
    uint64_t access_flags = err_code_to_access_flags(error_code); 
    if (!mm_check_access(mm, fault_addr, access_flags)) {
        // Should be SIGSEGV but we don't have that yet
        return;
    }

    struct mem_region *region = mm_find_region(mm, fault_addr);
    if (region->flags & RP_STACK) 
        mm_expand_stack(mm, fault_addr);
    else if (region->flags & RP_HEAP) 
        mm_expand_heap(mm, fault_addr);
    
}
