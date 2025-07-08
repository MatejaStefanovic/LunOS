#ifndef __KERNEL_VIRTUAL_MEM_MANAGER_H
#define __KERNEL_VIRTUAL_MEM_MANAGER_H

#include <kernel/memutils.h>
#include <kernel/compiler.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE-1))

#define PTE_PRESENT         (1UL << 0)
#define PTE_WRITABLE        (1UL << 1)
#define PTE_USER            (1UL << 2)
#define PTE_WRITETHROUGH    (1UL << 3)
#define PTE_CACHE_DISABLE   (1UL << 4)
#define PTE_ACCESSED        (1UL << 5)
#define PTE_DIRTY           (1UL << 6)
#define PTE_PAT             (1UL << 7)
#define PTE_GLOBAL          (1UL << 8)
#define PTE_NX              (1UL << 63)

// On x86_64 we use 48 bits for a VA and it consists of
// 9 bits for PML4, 9 for PDP, 9 for PD and final 9 for PT 
// these macros get those indexes, the remaining 12 bits are 
// used for the page offset 
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDP_INDEX(addr)  (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)
#define PAGE_OFFSET(addr) ((addr) & 0xFFF)

// Get PTE entry from address
#define PTE_ADDR(pte) ((pte) & 0x000FFFFFFFFFF000UL)


struct page_table {
    page_table_entry entries[512];
} _page_aligned; 

struct addr_space{
    struct page_table *pml4;
    uint64_t total_pages;
    uint64_t flags;
};

int vmm_init(void);
struct addr_space *vmm_create_address_space(void);
void vmm_destroy_address_space(struct addr_space *as);

struct page_table *vmm_alloc_page_table(void);
void vmm_free_page_table(struct page_table *pt);
page_table_entry *vmm_walk_page_table(struct addr_space *as, virt_addr vaddr, bool create);


int vmm_map_page(struct addr_space *as,virt_addr vaddr, phys_addr paddr, uint64_t flags);
int vmm_unmap_page(struct addr_space *as, virt_addr vaddr);
int vmm_map_range(struct addr_space *as, virt_addr vaddr, 
        phys_addr paddr, uint64_t size, uint64_t flags);
int vmm_unmap_range(struct addr_space *as, virt_addr vaddr, uint64_t size);


// Address translation
phys_addr vmm_virt_to_phys(struct addr_space *as, virt_addr vaddr);
bool vmm_is_mapped(struct addr_space *as, virt_addr vaddr);
void vmm_switch_address_space(struct addr_space* as);

int vmm_handle_page_fault(struct addr_space* as, virt_addr fault_addr, 
                            uint64_t error_code);

struct addr_space *get_kernel_as(void);

static inline virt_addr vmm_page_align_up(virt_addr vaddr){
    return (vaddr + PAGE_SIZE - 1) & PAGE_MASK;
}

static inline virt_addr vmm_page_align_down(virt_addr vaddr){
    return vaddr & PAGE_MASK;
}

static inline size_t vmm_pages_in_range(virt_addr start, virt_addr end){
    return (vmm_page_align_up(end) - vmm_page_align_down(start)) / PAGE_SIZE;
}

// Translation lookaside buffer stuff
static inline void vmm_flush_tlb(void) {
    __asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
}

static inline void vmm_flush_tlb_single(virt_addr vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void test_vmm(void);
#endif
