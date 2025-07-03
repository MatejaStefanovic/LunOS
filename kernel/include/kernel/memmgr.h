#ifndef __KERNEL_MEMMGR_H
#define __KERNEL_MEMMGR_H

/* This file is also a memory manager just on a higher abstraction level 
 * we use this to handle memory for processes and threads 
 * which we uniformally call tasks */
#include <kernel/vmm.h>

#define STACK_SIZE (8 * 1024 * 1024)    // 8MB stack
#define STACK_TOP 0x00007FFFFFFFFFFF

#define HEAP_GROW_SIZE (64 * 1024)   
#define HEAP_GROW_ORDER 4               // 2^4 = 16 pages = 64KiB
#define HEAP_SIZE (1024 * 1024 * 1024)  // 1GB heap 
#define GUARD_SIZE PAGE_SIZE 

// Page fault error code bits 
#define PF_PRESENT    (1 << 0)  // Page was present (1) or not present (0)
#define PF_WRITE      (1 << 1)  // Write (1) or read (0) access
#define PF_USER       (1 << 2)  // User mode (1) or supervisor mode (0)
#define PF_RESERVED   (1 << 3)  // Reserved bit violation
#define PF_INSTR      (1 << 4)  // Instruction fetch (1) or data access (0)

// Region permission flags
#define RP_READ     (1 << 0)
#define RP_WRITE    (1 << 1) 
#define RP_EXEC     (1 << 2)
#define RP_HEAP     (1 << 3)  // Special heap region
#define RP_STACK    (1 << 4)  // Special stack region
#define RP_SHARED   (1 << 5)  // Shared between processes

struct mem_region{
    virt_addr start;
    virt_addr end;
    uint64_t flags;
    struct mem_region* next;
};

struct mem_descriptor{
    struct addr_space *as;
    // Regions such as text, data, stack, heap sections and others (linked list)
    struct mem_region *regions; 

    virt_addr brk;
    virt_addr mmap_base;

    uint64_t total_vm;
    uint64_t rss; // Resident set size (how many pages in RAM the task has)
};

struct mem_descriptor* mm_alloc(void);
void mm_free(struct mem_descriptor *mm);
struct mem_descriptor* mm_copy(struct mem_descriptor *old_mm);  // for fork()

int mm_setup_executable(struct mem_descriptor *mm, virt_addr code_start, 
                       virt_addr code_end, virt_addr data_end);

// brk() is for heap 
virt_addr mm_brk(struct mem_descriptor *mm, virt_addr new_brk);

struct mem_region* mm_find_region(struct mem_descriptor *mm, virt_addr addr);
int mm_add_region(struct mem_descriptor *mm, virt_addr start, 
                                virt_addr end, uint64_t flags);
int mm_remove_region(struct mem_descriptor *mm, virt_addr start, virt_addr end);

// for mmap()
int mm_munmap(struct mem_descriptor *mm, virt_addr addr, size_t len);
virt_addr mm_mmap(struct mem_descriptor *mm, virt_addr addr, size_t len, 
                                                    int prot, int flags);

bool mm_check_access(struct mem_descriptor *mm, virt_addr addr, uint64_t flags);

int mm_expand_stack(struct mem_descriptor *mm, virt_addr fault_addr);
int mm_expand_heap(struct mem_descriptor *mm, virt_addr fault_addr);
void mm_page_fault_handler(uint64_t fault_addr, uint64_t error_code);

#endif
