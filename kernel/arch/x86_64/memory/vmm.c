#include <kernel/vmm.h>

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

static struct page_table_t* alloc_page_table(void){
    // TODO!!!
    return NULL;
}

static void free_page_table(struct page_table_t *pt){
    if(!pt)
        return;

    // TODODO
}

int vmm_init(){
    
    hhdm_offset = get_hhdm_offset();

    // TODOOOOOOOOO
    current_pml4 = get_current_pml4();
    
    if(!current_pml4){
        kprintf("PML4 is NULL\n");
        return 0;
    }
    
    return 1;
}

pte_t* vmm_walk_page_table(vaddr_t vaddr, bool create) {
    struct page_table_t* pml4 = get_current_pml4();
    if (!pml4) return NULL;
    
    struct page_table_t* current = pml4;
    
    // Walk through PML4 -> PDP -> PD -> PT
    uint32_t indices[] = {
        PML4_INDEX(vaddr),
        PDP_INDEX(vaddr),
        PD_INDEX(vaddr),
        PT_INDEX(vaddr)
    };
    
    for (int level = 0; level < 4; level++) {
        uint32_t idx = indices[level];
        pte_t* entry = &current->entries[idx];
        
        if (level == 3) {
            // Last level - return pointer to PTE
            return entry;
        }
        
        if (!(*entry & PTE_PRESENT)) {
            if (!create) return NULL;
            
            // Allocate new page table from our pool
            struct page_table_t* new_pt = alloc_page_table();
            if (!new_pt) return NULL;
            
            // Convert virtual address to physical
            paddr_t phys = (paddr_t)new_pt - hhdm_offset;
            *entry = phys | PTE_PRESENT | PTE_WRITABLE;
            
            // If this is for user space, add user flag
            if (vaddr < 0x800000000000UL) {
                *entry |= PTE_USER;
            }
        }
        
        // Move to next level
        paddr_t next_phys = PTE_ADDR(*entry);
        current = (struct page_table_t*)(next_phys + hhdm_offset);
    }
    
    return NULL; 
}

int vmm_map_page(vaddr_t vaddr, paddr_t paddr, uint64_t flags){
    vaddr = vmm_page_align_down(vaddr); 
    paddr = vmm_page_align_down(paddr);

    pte_t *pte = vmm_walk_page_table(vaddr, true);
    if(!pte)
        return -1;

    if(*pte & PTE_PRESENT)
        return -1; // Already mapped !?
    *pte = paddr | flags | PTE_PRESENT;
    
    vmm_flush_tlb();

    return 0;
}

int vmm_map_range(vaddr_t vaddr, paddr_t paddr, size_t size, uint64_t flags) {
    if (size == 0) 
        return -1;
    
    vaddr_t vstart = vmm_page_align_down(vaddr);
    vaddr_t vend = vmm_page_align_up(vaddr + size);
    paddr_t pstart = vmm_page_align_down(paddr);

    for (vaddr_t v = vstart, p = pstart; v < vend; v += PAGE_SIZE, p += PAGE_SIZE) {
        if (vmm_map_page(v, p, flags) != 0) {
            // Rollback on failure
            return -1;
        }
    }

    return 0;
}

paddr_t vmm_virt_to_phys(vaddr_t vaddr) {
    pte_t* pte = vmm_walk_page_table(vaddr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return 0;
    }
    
    return PTE_ADDR(*pte) | PAGE_OFFSET(vaddr);
}

bool vmm_is_mapped(vaddr_t vaddr) {
    return vmm_virt_to_phys(vaddr) != 0;
}













// Additional test to check page table walking
void debug_page_table_walk() {
    kprintf("=== Page Table Walk Debug ===\n");
    
    vaddr_t vaddr = 0xFFFF800001000000UL;
    
    kprintf("Testing page table walk for vaddr: 0x%lx\n", vaddr);
    kprintf("PML4 index: %lu\n", PML4_INDEX(vaddr));
    kprintf("PDP index: %lu\n", PDP_INDEX(vaddr));
    kprintf("PD index: %lu\n", PD_INDEX(vaddr));
    kprintf("PT index: %lu\n", PT_INDEX(vaddr));
    
    // Walk without creating
    pte_t *pte_before = vmm_walk_page_table(vaddr, false);
    kprintf("PTE before mapping: %p\n", pte_before);
    
    if (pte_before) {
        kprintf("PTE value before mapping: 0x%lx\n", *pte_before);
    }
    
    // Walk with creating
    pte_t *pte_after = vmm_walk_page_table(vaddr, true);
    kprintf("PTE after creating path: %p\n", pte_after);
    
    if (pte_after) {
        kprintf("PTE value after creating path: 0x%lx\n", *pte_after);
        
        // Now set the mapping manually and test
        paddr_t paddr = 0x100000;
        *pte_after = paddr | PTE_PRESENT | PTE_WRITABLE;
        kprintf("Set PTE to: 0x%lx\n", *pte_after);
        
        // Flush TLB
        vmm_flush_tlb();
        
        // Test the mapping
        uint64_t *virt_ptr = (uint64_t*)vaddr;
        uint64_t *hhdm_ptr = (uint64_t*)(paddr + hhdm_offset);
        
        // Write via HHDM, read via virtual
        *hhdm_ptr = 0xDEADBEEF;
        kprintf("Wrote 0xDEADBEEF via HHDM, reading via virtual: 0x%lx\n", *virt_ptr);
        
        // Write via virtual, read via HHDM
        *virt_ptr = 0xCAFEBABE;
        kprintf("Wrote 0xCAFEBABE via virtual, reading via HHDM: 0x%lx\n", *hhdm_ptr);
    }
}

// Test VMM without relying on the pool allocation
void test_vmm_without_pool() {
    kprintf("=== VMM Test Without Pool ===\n");
    
    if (!vmm_init()) {
        kprintf("VMM init failed!\n");
        return;
    }
    
    // Use an address that might already have page table entries
    // Try mapping in the kernel space near where we know tables exist
    vaddr_t vaddr = 0xFFFF800000500000UL;  // Different kernel address
    paddr_t paddr = 0x500000;  // 5MB physical
    
    kprintf("Testing with vaddr: 0x%lx -> paddr: 0x%lx\n", vaddr, paddr);
    
    // First, check if this virtual address already has intermediate page tables
    pte_t *pte = vmm_walk_page_table(vaddr, false);  // Don't create
    if (pte) {
        kprintf("Page table path already exists!\n");
        kprintf("Current PTE value: 0x%lx\n", *pte);
        
        // Directly set the PTE
        *pte = paddr | PTE_PRESENT | PTE_WRITABLE;
        kprintf("Set PTE to: 0x%lx\n", *pte);
        
        // Flush TLB
        vmm_flush_tlb();
        
        // Test the mapping
        uint64_t *hhdm_ptr = (uint64_t*)(paddr + hhdm_offset);
        uint64_t *virt_ptr = (uint64_t*)vaddr;
        
        // Write via HHDM, read via virtual
        *hhdm_ptr = 0x1234567890ABCDEF;
        kprintf("Wrote via HHDM, reading via virtual: 0x%lx\n", *virt_ptr);
        
        // Write via virtual, read via HHDM  
        *virt_ptr = 0xFEDCBA0987654321;
        kprintf("Wrote via virtual, reading via HHDM: 0x%lx\n", *hhdm_ptr);
        
        return;
    }
    
    kprintf("No existing page table path found\n");
    
    // Try a different approach - use an existing mapped region
    // Check what's already mapped in high kernel memory
    for (vaddr_t test_addr = 0xFFFF800000000000UL; 
         test_addr < 0xFFFF800010000000UL; 
         test_addr += 0x1000000) {  // 16MB steps
        
        if (vmm_is_mapped(test_addr)) {
            kprintf("Found existing mapping at: 0x%lx\n", test_addr);
            paddr_t existing_phys = vmm_virt_to_phys(test_addr);
            kprintf("  Maps to physical: 0x%lx\n", existing_phys);
            
            // Test this existing mapping
            uint64_t *virt_ptr = (uint64_t*)test_addr;
            uint64_t *hhdm_ptr = (uint64_t*)(existing_phys + hhdm_offset);
            
            uint64_t orig_val = *virt_ptr;
            kprintf("  Original value via virtual: 0x%lx\n", orig_val);
            kprintf("  Same value via HHDM: 0x%lx\n", *hhdm_ptr);
            
            if (orig_val == *hhdm_ptr) {
                kprintf("  HHDM matches virtual - mapping is correct!\n");
                
                // Try writing (carefully, save original)
                *virt_ptr = 0xABCDEF12345678;
                if (*hhdm_ptr == 0xABCDEF12345678) {
                    kprintf("  Write test PASSED!\n");
                } else {
                    kprintf("  Write test FAILED!\n");
                }
                
                // Restore original value
                *virt_ptr = orig_val;
                return;
            }
            break;
        }
    }
    
    kprintf("No suitable existing mappings found for testing\n");
}
