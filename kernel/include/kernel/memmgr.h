#ifndef __KERNEL_MEMMGR_H
#define __KERNEL_MEMMGR_H

/* This file is also a memory manager just on a higher abstraction level 
 * we use this to handle memory for processes and threads 
 * which we uniformally call tasks */
#include <kernel/vmm.h>

// Region permission flags
#define RP_READ     (1 << 0)
#define RP_WRITE    (1 << 1) 
#define RP_EXEC     (1 << 2)
#define RP_HEAP     (1 << 3)  // Special heap region
#define RP_STACK    (1 << 4)  // Special stack region
#define RP_SHARED   (1 << 5)  // Shared between processes

struct mem_region_t{
    vaddr_t start;
    vaddr_t end;
    uint64_t flags;
    struct mem_region_t* next;
};

struct mem_descriptor_t{
    struct addr_space_t *as;
    // Regions such as text, data, stack, heap sections and others (linked list)
    struct mem_region_t *regions; 

    vaddr_t brk;
    vaddr_t mmap_base;

    uint64_t total_vm;
    uint64_t rss; // Resident set size (how many pages in RAM the task has)
};

struct mem_descriptor_t* mm_alloc(void);
void mm_free(struct mem_descriptor_t *mm);
struct mem_descriptor_t* mm_copy(struct mem_descriptor_t *old_mm);  // for fork()

int mm_setup_executable(struct mem_descriptor_t *mm, vaddr_t code_start, 
                       vaddr_t code_end, vaddr_t data_end);

// brk() is for heap 
vaddr_t mm_brk(struct mem_descriptor_t *mm, vaddr_t new_brk);

struct mem_region_t* mm_find_region(struct mem_descriptor_t *mm, vaddr_t addr);
int mm_add_region(struct mem_descriptor_t *mm, vaddr_t start, 
                                vaddr_t end, uint64_t flags);
int mm_remove_region(struct mem_descriptor_t *mm, vaddr_t start, vaddr_t end);

// for mmap()
vaddr_t mm_mmap(struct mem_descriptor_t *mm, vaddr_t addr, size_t len,
                                            int prot, int flags);
int mm_munmap(struct mem_descriptor_t *mm, vaddr_t addr, size_t len);

bool mm_check_access(struct mem_descriptor_t *mm, vaddr_t addr, uint64_t flags);

#endif
