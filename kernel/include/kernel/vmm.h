#ifndef __KERNEL_VIRTUAL_MEM_MANAGER_H
#define __KERNEL_VIRTUAL_MEM_MANAGER_H

#include <kernel/memutils.h>

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


struct page_table_t {
    pte_t entries[512];
} __attribute__((aligned(PAGE_SIZE)));

struct mem_region_t{
    vaddr_t start;
    vaddr_t end;
    uint64_t flags;
    struct mem_region_t* next;
};

struct addr_space_t{
    struct page_table_t *pml4;
    struct mem_region_t *regions;
    uint64_t total_pages;
    uint64_t flags;
};

int vmm_init(void);
struct addr_space_t *vmm_create_address_space(void);
void vmm_remove_address_space(struct addr_space_t *as);

struct page_table_t *vmm_alloc_page_table(void);
void vmm_free_page_table(struct page_table_t *pt);
pte_t *vmm_walk_page_table(struct addr_space_t *as, vaddr_t vaddr, bool create);


int vmm_map_page(struct addr_space_t *as,vaddr_t vaddr, paddr_t paddr, uint64_t flags);
int vmm_unmap_page(struct addr_space_t *as, vaddr_t vaddr);
int vmm_map_range(struct addr_space_t *as, vaddr_t vaddr, 
        paddr_t paddr, uint64_t size, uint64_t flags);
int vmm_unmap_range(struct addr_space_t *as, vaddr_t vaddr, uint64_t size);


// Address translation
paddr_t vmm_virt_to_phys(struct addr_space_t *as, vaddr_t vaddr);
bool vmm_is_mapped(struct addr_space_t *as, vaddr_t vaddr);
void vmm_switch_address_space(struct addr_space_t* as);

// Track memory regions
void vmm_add_region(struct addr_space_t *as, vaddr_t vaddr, paddr_t paddr, uint64_t size);
void vmm_remove_region(struct addr_space_t *as, vaddr_t vaddr);
struct mem_region_t *find_region(struct addr_space_t *as, vaddr_t vaddr);


int vmm_handle_page_fault(struct addr_space_t* as, vaddr_t fault_addr, 
                            uint64_t error_code);

// Debug functions
void vmm_dump_page_table_entry(vaddr_t vaddr);
void vmm_dump_regions(void);

static inline vaddr_t vmm_page_align_up(vaddr_t vaddr){
    return (vaddr + PAGE_SIZE - 1) & PAGE_MASK;
}

static inline vaddr_t vmm_page_align_down(vaddr_t vaddr){
    return vaddr & PAGE_MASK;
}

static inline size_t vmm_pages_in_range(vaddr_t start, vaddr_t end){
    return (vmm_page_align_up(end) - vmm_page_align_down(start)) / PAGE_SIZE;
}

// Translation lookaside buffer stuff
static inline void vmm_flush_tlb(void) {
    __asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
}

static inline void vmm_flush_tlb_single(vaddr_t vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void test_vmm(void);
/*
    Might be useful in the future Idk
    struct limine_kernel_address_request *ka_req = get_kernel_address_request();
    vaddr_t kernel_virt = ka_req->response->virtual_base;
    paddr_t kernel_phys = ka_req->response->physical_base;
    
    extern char _kernel_start[];
    extern char _kernel_end[];

    uint64_t kernel_size = (uint64_t)_kernel_end - (uint64_t)_kernel_start;
 
*/
#endif
