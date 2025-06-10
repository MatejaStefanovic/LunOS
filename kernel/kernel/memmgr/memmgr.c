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
    kprintf("Couldn't find region");
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

    vaddr_t stack_top = 0x00007FFFFFFFFFFF;
    vaddr_t stack_bottom = stack_top - STACK_SIZE + 1;
    vaddr_t guard_stack = stack_bottom - GUARD_SIZE;
    
    mm_add_region(mm, guard_stack, stack_bottom - 1, 0);
    mm_add_region(mm, stack_bottom, stack_top, RP_READ | RP_WRITE);
    
    return 0;
}



