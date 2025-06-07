#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <string.h>

static struct page_table_t* current_pml4 = NULL;
static uint64_t hhdm_offset;

static struct page_table_t* get_current_pml4(){
    if(!current_pml4){
        paddr_t cr3 = get_cr3();
        // We need to see where out higher half direct mapping starts and we 
        // get that info from limine
        current_pml4 = (struct page_table_t*)(cr3 + hhdm_offset);
    }
    return current_pml4; 
}

int vmm_init(){
    hhdm_offset = get_hhdm_offset();
    current_pml4 = get_current_pml4();
    
    if(!current_pml4){
        kprintf("PML4 is NULL\n");
        return -1;
    }
    
    return 0;
}

struct page_table_t* vmm_alloc_page_table(void){
    paddr_t phys = pmm_alloc_page();
    if(phys == 0)
        return NULL;
    
    struct page_table_t *pt = (struct page_table_t*)(phys + hhdm_offset);
    memset(pt, 0, PAGE_SIZE);

    return pt;
}

static void free_page_table(struct page_table_t *pt){
    if(!pt){
        KERROR("Tried to free a NULL ptr\n");
        return;
    }
    uint64_t phys = (uint64_t)pt - hhdm_offset;
    pmm_free_page(phys);
}

pte_t* vmm_walk_page_table(struct addr_space_t *as, vaddr_t vaddr, bool create) {
    if(!as || !as->pml4)
        return NULL;

    struct page_table_t* current = as->pml4;
    
    // Walk through PML4 -> PDP -> PD -> PT
    uint32_t indices[] = {
        PML4_INDEX(vaddr),
        PDP_INDEX(vaddr),
        PD_INDEX(vaddr),
        PT_INDEX(vaddr)
    };
    
    for (int level = 0; level < 4; ++level) {
        uint32_t idx = indices[level];
        pte_t* entry = &current->entries[idx];
        
        if (level == 3) {
            // Last level - return pointer to PTE
            return entry;
        }
        
        if (!(*entry & PTE_PRESENT)) {
            if (!create) 
                return NULL;
            
            struct page_table_t* new_pt = vmm_alloc_page_table();
            if (!new_pt) 
                return NULL;
            
            // We only ever map user space programs as kernel uses HHDM
            // hence why we always add PTE_USER
            paddr_t phys = (paddr_t)new_pt - hhdm_offset;
            *entry = phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;    
        }
        
        // Move to next level
        paddr_t next_phys = PTE_ADDR(*entry);
        current = (struct page_table_t*)(next_phys + hhdm_offset);
    }
    
    return NULL; 
}

static int _vmm_map_page_no_flush(struct addr_space_t *as, vaddr_t vaddr, 
        paddr_t paddr, uint64_t flags){

    if(!as){
        KERROR("Cannot map page, virtual adress space is NULL\n");
        return -1;
    }

    vaddr = vmm_page_align_down(vaddr); 
    paddr = vmm_page_align_down(paddr);

    pte_t *pte = vmm_walk_page_table(as, vaddr, true);
    
    if(!pte){
        KERROR("Cannot map page, page table entry is NULL\n");
        return -1;
    }
    
    if(*pte & PTE_PRESENT){
        KERROR("Cannot map page, this page table entry is already PRESENT\n");
        return -1;
    }

    *pte = paddr | flags | PTE_PRESENT;
    as->total_pages++;

    return 0;
}

int vmm_map_page(struct addr_space_t *as, vaddr_t vaddr, 
        paddr_t paddr, uint64_t flags){

    if(!as){
        KERROR("Cannot map page, virtual adress space is NULL\n");
        return -1;
    }

    vaddr = vmm_page_align_down(vaddr); 
    paddr = vmm_page_align_down(paddr);

    pte_t *pte = vmm_walk_page_table(as, vaddr, true);
    
    if(!pte){
        KERROR("Cannot map page, page table entry is NULL\n");
        return -1;
    }
    
    if(*pte & PTE_PRESENT){
        KERROR("Cannot map page, this page table entry is already PRESENT\n");
        return -1;
    }

    *pte = paddr | flags | PTE_PRESENT;
    vmm_flush_tlb_single(vaddr);
    as->total_pages++;

    return 0;
}

int vmm_map_range(struct addr_space_t *as, vaddr_t vaddr, 
        paddr_t paddr, size_t size, uint64_t flags) {
    
    if(!as){
        KERROR("Cannot map pages, virtual adress space is NULL\n");
        return -1;
    }

    if (size == 0){ 
        KERROR("Tried to allocate nothing\n");
        return -1;
    }
    
    vaddr_t vstart = vmm_page_align_down(vaddr);
    vaddr_t vend = vmm_page_align_up(vaddr + size);
    paddr_t pstart = vmm_page_align_down(paddr);

    for (vaddr_t v = vstart, p = pstart; v < vend; v += PAGE_SIZE, p += PAGE_SIZE) {
    if (_vmm_map_page_no_flush(as, v, p, flags) != 0) {
            // Rollback on failure
            // TODO: add vmm_unmap_range();
            return -1;
        }
    }
    vmm_flush_tlb();
    return 0;
}

static int _vmm_unmap_page_no_flush(struct addr_space_t* as, vaddr_t vaddr) {
    if (!as) return -1;
    
    vaddr = vmm_page_align_down(vaddr);
    
    pte_t* pte = vmm_walk_page_table(as, vaddr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return -1; // Not mapped
    }
    
    *pte = 0;
    
    as->total_pages--;
    
    return 0;
}

int vmm_unmap_page(struct addr_space_t *as, vaddr_t vaddr) {
    if (!as) return -1;
    
    vaddr = vmm_page_align_down(vaddr);
    
    pte_t* pte = vmm_walk_page_table(as, vaddr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return -1; // Not mapped
    }
    
    *pte = 0;
    vmm_flush_tlb_single(vaddr);
    
    as->total_pages--;
    
    return 0;
}

int vmm_unmap_range(struct addr_space_t *as, vaddr_t vaddr, size_t size) {
    if (!as || size == 0) return -1;
    
    vaddr_t vstart = vmm_page_align_down(vaddr);
    vaddr_t vend = vmm_page_align_up(vaddr + size);
    
    for (vaddr_t v = vstart; v < vend; v += PAGE_SIZE) {
        _vmm_unmap_page_no_flush(as, v);
    }
    vmm_flush_tlb(); 
    return 0;
}

paddr_t vmm_virt_to_phys(struct addr_space_t *as, vaddr_t vaddr) {
    if(!as)
        return 0;

    pte_t* pte = vmm_walk_page_table(as, vaddr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return 0;
    }
    
    return PTE_ADDR(*pte) | PAGE_OFFSET(vaddr);
}

bool vmm_is_mapped(struct addr_space_t *as, vaddr_t vaddr) {
    return vmm_virt_to_phys(as, vaddr) != 0;
}
