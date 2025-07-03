#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <tests/vmm_tests.h>
// Test function to validate your VMM implementation
static int test_vmm_basic(void) {
    kprintf("=== VMM Basic Test ===\n");
    
    // Create a new address space
    struct addr_space test_as;
    test_as.pml4 = vmm_alloc_page_table();
    test_as.total_pages = 0;
    
    if (!test_as.pml4) {
        KERROR("FAIL: Could not allocate PML4\n");
        return -1;
    }
    KSUCCESS("PML4 allocated at virtual: %p\n", test_as.pml4);
    
    // Test 1: Map a single page
    virt_addr test_vaddr = 0x400000;  // 4MB virtual address
    phys_addr test_paddr = pmm_alloc_page();
    
    if (test_paddr == 0) {
        KERROR("FAIL: Could not allocate physical page\n");
        return -1;
    }
    KSUCCESS("Physical page allocated: 0x%lx\n", test_paddr);
    
    // Map the page
    int result = vmm_map_page(&test_as, test_vaddr, test_paddr, PTE_WRITABLE | PTE_USER);
    if (result != 0) {
        KERROR("FAIL: vmm_map_page returned %d\n", result);
        return -1;
    }
    KSUCCESS("Page mapped successfully\n");
    
    // Test 2: Verify mapping exists
    if (!vmm_is_mapped(&test_as, test_vaddr)) {
        KERROR("FAIL: vmm_is_mapped returned false\n");
        return -1;
    }
    KSUCCESS("Mapping verified\n");
    
    // Test 3: Test virtual to physical translation
    phys_addr resolved_paddr = vmm_virt_to_phys(&test_as, test_vaddr);
    if (resolved_paddr != test_paddr) {
        KERROR("FAIL: Translation mismatch. Expected: 0x%lx, Got: 0x%lx\n", 
                test_paddr, resolved_paddr);
        return -1;
    }
    KSUCCESS("Virtual to physical translation correct\n");
    
    // Test 4: Test page walk function directly
    page_table_entry* pte = vmm_walk_page_table(&test_as, test_vaddr, false);
    if (!pte) {
        KERROR("FAIL: vmm_walk_page_table returned NULL\n");
        return -1;
    }
    if (!(*pte & PTE_PRESENT)) {
        KERROR("FAIL: PTE is not marked present\n");
        return -1;
    }
    KSUCCESS("Page walk function working\n");
    
    // Test 5: Test with offset within page
    virt_addr offset_vaddr = test_vaddr + 0x123;  // Add some offset
    phys_addr offset_paddr = vmm_virt_to_phys(&test_as, offset_vaddr);
    phys_addr expected_offset_paddr = test_paddr + 0x123;
    
    if (offset_paddr != expected_offset_paddr) {
        KERROR("FAIL: Offset translation wrong. Expected: 0x%lx, Got: 0x%lx\n",
                expected_offset_paddr, offset_paddr);
        return -1;
    }
    KSUCCESS("Offset translation working\n");
    
    kprintf("=== All basic tests passed! ===\n\n");
    return 0;
}

// Test range mapping
static int test_vmm_range(void) {
    kprintf("=== VMM Range Test ===\n");
    
    struct addr_space test_as;
    test_as.pml4 = vmm_alloc_page_table();
    test_as.total_pages = 0;
    
    if (!test_as.pml4) {
        KERROR("FAIL: Could not allocate PML4\n");
        return -1;
    }
    
    // Allocate contiguous physical pages
    size_t num_pages = 4;
    phys_addr base_paddr = pmm_alloc_page();
    
    
    virt_addr base_vaddr = 0x800000;  // 8MB
    size_t total_size = num_pages * PAGE_SIZE;
    
    int result = vmm_map_range(&test_as, base_vaddr, base_paddr, total_size, 
                               PTE_WRITABLE | PTE_USER);
    
    if (result != 0) {
        KERROR("FAIL: vmm_map_range returned %d\n", result);
        return -1;
    }
    KSUCCESS("Range mapped successfully\n");
    
    // Verify all pages in range are mapped
    for (size_t i = 0; i < num_pages; i++) {
        virt_addr check_vaddr = base_vaddr + (i * PAGE_SIZE);
        if (!vmm_is_mapped(&test_as, check_vaddr)) {
            KERROR("FAIL: Page %lu not mapped at 0x%lx\n", i, check_vaddr);
            return -1;
        }
    }
    KSUCCESS("All pages in range are mapped\n");
    
    kprintf("=== Range test passed! ===\n\n");
    return 0;
}

// Test unmapping functionality
static int test_vmm_unmap(void) {
    kprintf("=== VMM Unmap Test ===\n");
    
    struct addr_space test_as;
    test_as.pml4 = vmm_alloc_page_table();
    test_as.total_pages = 0;
    
    if (!test_as.pml4) {
        KERROR("FAIL: Could not allocate PML4\n");
        return -1;
    }
    
    // Test 1: Single page unmap
    virt_addr test_vaddr = 0x500000;
    phys_addr test_paddr = pmm_alloc_page();
    
    if (test_paddr == 0) {
        KERROR("FAIL: Could not allocate physical page\n");
        return -1;
    }
    
    // Map the page
    if (vmm_map_page(&test_as, test_vaddr, test_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: vmm_map_page failed\n");
        return -1;
    }
    
    // Verify it's mapped
    if (!vmm_is_mapped(&test_as, test_vaddr)) {
        KERROR("FAIL: Page not mapped after mapping\n");
        return -1;
    }
    KSUCCESS("Single page mapped successfully\n");
    
    // Unmap the page
    if (vmm_unmap_range(&test_as, test_vaddr, PAGE_SIZE) != 0) {
        KERROR("FAIL: vmm_unmap_range failed\n");
        return -1;
    }
    
    // Verify it's no longer mapped
    if (vmm_is_mapped(&test_as, test_vaddr)) {
        KERROR("FAIL: Page still mapped after unmapping\n");
        return -1;
    }
    KSUCCESS("Single page unmapped successfully\n");
    
    // Test 2: Range unmap
    size_t num_pages = 5;
    virt_addr base_vaddr = 0x600000;
    size_t total_size = num_pages * PAGE_SIZE;
    
    // Map multiple pages
    for (size_t i = 0; i < num_pages; i++) {
        virt_addr vaddr = base_vaddr + (i * PAGE_SIZE);
        phys_addr paddr = pmm_alloc_page();
        
        if (paddr == 0) {
            KERROR("FAIL: Could not allocate physical page %lu\n", i);
            return -1;
        }
        
        if (vmm_map_page(&test_as, vaddr, paddr, PTE_WRITABLE | PTE_USER) != 0) {
            KERROR("FAIL: vmm_map_page failed for page %lu\n", i);
            return -1;
        }
    }
    
    // Verify all pages are mapped
    for (size_t i = 0; i < num_pages; i++) {
        virt_addr vaddr = base_vaddr + (i * PAGE_SIZE);
        if (!vmm_is_mapped(&test_as, vaddr)) {
            KERROR("FAIL: Page %lu not mapped at 0x%lx\n", i, vaddr);
            return -1;
        }
    }
    KSUCCESS("Range of %lu pages mapped successfully\n", num_pages);
    
    // Unmap the entire range
    if (vmm_unmap_range(&test_as, base_vaddr, total_size) != 0) {
        KERROR("FAIL: vmm_unmap_range failed for range\n");
        return -1;
    }
    
    // Verify all pages are unmapped
    for (size_t i = 0; i < num_pages; i++) {
        virt_addr vaddr = base_vaddr + (i * PAGE_SIZE);
        if (vmm_is_mapped(&test_as, vaddr)) {
            KERROR("FAIL: Page %lu still mapped after range unmap at 0x%lx\n", i, vaddr);
            return -1;
        }
    }
    KSUCCESS("Range of %lu pages unmapped successfully\n", num_pages);
    
    // Test 3: Partial range unmap
    virt_addr partial_base = 0x700000;
    size_t partial_pages = 6;
    
    // Map pages
    for (size_t i = 0; i < partial_pages; i++) {
        virt_addr vaddr = partial_base + (i * PAGE_SIZE);
        phys_addr paddr = pmm_alloc_page();
        
        if (vmm_map_page(&test_as, vaddr, paddr, PTE_WRITABLE | PTE_USER) != 0) {
            KERROR("FAIL: vmm_map_page failed for partial test page %lu\n", i);
            return -1;
        }
    }
    
    // Unmap middle 3 pages (pages 1, 2, 3)
    virt_addr unmap_start = partial_base + PAGE_SIZE;
    size_t unmap_size = 3 * PAGE_SIZE;
    
    if (vmm_unmap_range(&test_as, unmap_start, unmap_size) != 0) {
        KERROR("FAIL: vmm_unmap_range failed for partial range\n");
        return -1;
    }
    
    // Verify first and last pages are still mapped
    if (!vmm_is_mapped(&test_as, partial_base)) {
        KERROR("FAIL: First page incorrectly unmapped\n");
        return -1;
    }
    if (!vmm_is_mapped(&test_as, partial_base + (4 * PAGE_SIZE))) {
        KERROR("FAIL: Page 4 incorrectly unmapped\n");
        return -1;
    }
    if (!vmm_is_mapped(&test_as, partial_base + (5 * PAGE_SIZE))) {
        KERROR("FAIL: Last page incorrectly unmapped\n");
        return -1;
    }
    
    // Verify middle pages are unmapped
    for (size_t i = 1; i <= 3; i++) {
        virt_addr vaddr = partial_base + (i * PAGE_SIZE);
        if (vmm_is_mapped(&test_as, vaddr)) {
            KERROR("FAIL: Page %lu should be unmapped but isn't at 0x%lx\n", i, vaddr);
            return -1;
        }
    }
    KSUCCESS("Partial range unmap working correctly\n");
    
    // Test 4: Unaligned unmap (should align properly)
    virt_addr aligned_base = 0x600000;
    phys_addr aligned_paddr = pmm_alloc_page();
    
    if (vmm_map_page(&test_as, aligned_base, aligned_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: vmm_map_page failed for alignment test\n");
        return -1;
    }
    
    // Unmap with unaligned address and size - should still unmap the whole page
    virt_addr unaligned_addr = aligned_base + 0x100;  // Offset into page
    
    if (vmm_unmap_page(&test_as, unaligned_addr) != 0) {
        KERROR("FAIL: vmm_unmap_range failed for unaligned test\n");
        return -1;
    }
        
    // The entire page should be unmapped despite unaligned parameters
    if (vmm_is_mapped(&test_as, aligned_base)) {
        KERROR("FAIL: Page should be unmapped after unaligned unmap\n");
        return -1;
    }
    KSUCCESS("Unaligned unmap handled correctly\n");
    
    kprintf("=== Unmap tests passed! ===\n\n");
    return 0;
}

// Test memory access behavior after unmapping
/*
static int test_vmm_unmap_memory_access(void) {
    kprintf("=== VMM Unmap Memory Access Test ===\n");
    
    struct addr_space test_as;
    test_as.pml4 = vmm_alloc_page_table();
    test_as.total_pages = 0;
    
    if (!test_as.pml4) {
        KERROR("FAIL: Could not allocate PML4\n");
        return -1;
    }
    
    // Test 1: Verify physical memory content persists after unmap
    virt_addr test_vaddr = 0x500000;
    phys_addr test_paddr = pmm_alloc_page();
    
    if (test_paddr == 0) {
        KERROR("FAIL: Could not allocate physical page\n");
        return -1;
    }
    
    // Write test pattern to physical memory via HHDM
    uint64_t *phys_ptr = (uint64_t *)(test_paddr + get_hhdm_offset());
    uint64_t test_pattern = 0xABCDEF1234567890ULL;
    *phys_ptr = test_pattern;
    
    kprintf("Wrote test pattern 0x%lx to physical 0x%lx\n", test_pattern, test_paddr);
    
    // Map the page
    if (vmm_map_page(&test_as, test_vaddr, test_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: vmm_map_page failed\n");
        return -1;
    }
    
    // Verify mapping works and translation is correct
    if (!vmm_is_mapped(&test_as, test_vaddr)) {
        KERROR("FAIL: Page not mapped after mapping\n");
        return -1;
    }
    
    phys_addr resolved_paddr = vmm_virt_to_phys(&test_as, test_vaddr);
    if (resolved_paddr != test_paddr) {
        KERROR("FAIL: Translation incorrect before unmap\n");
        return -1;
    }
    KSUCCESS("Page mapped and translation working\n");
    
    // Unmap the page
    if (vmm_unmap_range(&test_as, test_vaddr, PAGE_SIZE) != 0) {
        KERROR("FAIL: vmm_unmap_range failed\n");
        return -1;
    }
    
    // Verify page is unmapped
    if (vmm_is_mapped(&test_as, test_vaddr)) {
        KERROR("FAIL: Page still mapped after unmap\n");
        return -1;
    }
    KSUCCESS("Page successfully unmapped\n");
    
    // Verify translation no longer works
    phys_addr unmapped_paddr = vmm_virt_to_phys(&test_as, test_vaddr);
    if (unmapped_paddr != 0) {  // Assuming vmm_virt_to_phys returns 0 for unmapped pages
        kprintf("INFO: vmm_virt_to_phys returned 0x%lx for unmapped page (implementation specific)\n", unmapped_paddr);
    } else {
        KSUCCESS("Translation correctly returns 0 for unmapped page\n");
    }
    
    // Verify physical memory content is unchanged (via HHDM)
    uint64_t read_pattern = *phys_ptr;
    if (read_pattern != test_pattern) {
        KERROR("FAIL: Physical memory corrupted after unmap. Expected: 0x%lx, Got: 0x%lx\n", 
                test_pattern, read_pattern);
        return -1;
    }
    KSUCCESS("Physical memory content preserved after unmap\n");
    
    // Test 2: Verify page table entries are cleared
    page_table_entry *pte = vmm_walk_page_table(&test_as, test_vaddr, false);
    if (pte && (*pte & PTE_PRESENT)) {
        KERROR("FAIL: PTE still marked present after unmap\n");
        kprintf("PTE value: 0x%lx\n", *pte);
        return -1;
    }
    KSUCCESS("Page table entry properly cleared\n");
    
    // Test 3: Test remapping to same virtual address works
    phys_addr new_paddr = pmm_alloc_page();
    if (new_paddr == 0) {
        KERROR("FAIL: Could not allocate new physical page\n");
        return -1;
    }
    
    // Write different pattern to new physical page
    uint64_t *new_phys_ptr = (uint64_t *)(new_paddr + get_hhdm_offset());
    uint64_t new_pattern = 0x1122334455667788ULL;
    *new_phys_ptr = new_pattern;
    
    // Remap the same virtual address to new physical page
    if (vmm_map_page(&test_as, test_vaddr, new_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: Failed to remap virtual address\n");
        return -1;
    }
    
    // Verify new mapping works
    if (!vmm_is_mapped(&test_as, test_vaddr)) {
        KERROR("FAIL: Remapped page not showing as mapped\n");
        return -1;
    }
    
    phys_addr remapped_paddr = vmm_virt_to_phys(&test_as, test_vaddr);
    if (remapped_paddr != new_paddr) {
        KERROR("FAIL: Remapped translation incorrect. Expected: 0x%lx, Got: 0x%lx\n",
                new_paddr, remapped_paddr);
        return -1;
    }
    KSUCCESS("Successfully remapped virtual address to new physical page\n");
    
    // Test 4: Test range unmap with memory access verification
    size_t range_pages = 3;
    virt_addr range_base = 0x600000;
    phys_addr range_paddrs[3];
    uint64_t range_patterns[3] = {0x1111111111111111ULL, 0x2222222222222222ULL, 0x3333333333333333ULL};
    
    // Map and initialize range
    for (size_t i = 0; i < range_pages; i++) {
        range_paddrs[i] = pmm_alloc_page();
        if (range_paddrs[i] == 0) {
            KERROR("FAIL: Could not allocate physical page %lu for range test\n", i);
            return -1;
        }
        
        virt_addr vaddr = range_base + (i * PAGE_SIZE);
        
        // Write pattern to physical memory
        phys_ptr = (uint64_t *)(range_paddrs[i] + get_hhdm_offset());
        *phys_ptr = range_patterns[i];
        
        // Map the page
        if (vmm_map_page(&test_as, vaddr, range_paddrs[i], PTE_WRITABLE | PTE_USER) != 0) {
            KERROR("FAIL: vmm_map_page failed for range page %lu\n", i);
            return -1;
        }
    }
    
    // Verify all pages are mapped
    for (size_t i = 0; i < range_pages; i++) {
        virt_addr vaddr = range_base + (i * PAGE_SIZE);
        if (!vmm_is_mapped(&test_as, vaddr)) {
            KERROR("FAIL: Range page %lu not mapped\n", i);
            return -1;
        }
    }
    KSUCCESS("Range of %lu pages mapped successfully\n", range_pages); 
    
    // Unmap the entire range
    if (vmm_unmap_range(&test_as, range_base, range_pages * PAGE_SIZE) != 0) {
        KERROR("FAIL: vmm_unmap_range failed for range\n");
        return -1;
    }
    
    // Verify all pages are unmapped
    for (size_t i = 0; i < range_pages; i++) {
        virt_addr vaddr = range_base + (i * PAGE_SIZE);
        if (vmm_is_mapped(&test_as, vaddr)) {
            KERROR("FAIL: Range page %lu still mapped after unmap\n", i);
            return -1;
        }
    }
    
    // Verify physical memory content is preserved
    for (size_t i = 0; i < range_pages; i++) {
        phys_ptr = (uint64_t *)(range_paddrs[i] + get_hhdm_offset());
        read_pattern = *phys_ptr;
        if (read_pattern != range_patterns[i]) {
            KERROR("FAIL: Physical memory %lu corrupted. Expected: 0x%lx, Got: 0x%lx\n",
                    i, range_patterns[i], read_pattern);
            return -1;
        }
    }
    KSUCCESS("All physical memory content preserved after range unmap\n\n");
    
    // Test 5: Controlled page fault test
    kprintf("=== Testing Page Fault on Unmapped Memory ===\n");
    
    // Set up a page for fault testing
    virt_addr fault_test_vaddr = 0x700000;
    phys_addr fault_test_paddr = pmm_alloc_page();
    
    if (fault_test_paddr == 0) {
        KERROR("FAIL: Could not allocate physical page for fault test\n");
        return -1;
    }
    
    // Map the page
    if (vmm_map_page(&test_as, fault_test_vaddr, fault_test_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: vmm_map_page failed for fault test\n");
        return -1;
    }
    
    // Write test data to verify we can access it while mapped
    uint64_t *fault_test_ptr = (uint64_t *)(fault_test_paddr + get_hhdm_offset());
    uint64_t fault_test_pattern = 0xDEADBEEFCAFEBEEF;
    *fault_test_ptr = fault_test_pattern;
    
    kprintf("Mapped page at virtual 0x%lx -> physical 0x%lx\n", fault_test_vaddr, fault_test_paddr);
    
    // Unmap the page
    if (vmm_unmap_range(&test_as, fault_test_vaddr, PAGE_SIZE) != 0) {
        KERROR("FAIL: vmm_unmap_range failed for fault test\n");
        return -1;
    }
    
    // Verify it's unmapped
    if (vmm_is_mapped(&test_as, fault_test_vaddr)) {
        KERROR("FAIL: Page still mapped after unmap in fault test\n");
        return -1;
    }
    
    kprintf("Page unmapped. Now testing page fault behavior...\n");
    
    // NOTE: We can't actually switch to the test address space and cause a real page fault
    // in this test environment because:
    // 1. We're running in kernel mode with the kernel's page table
    // 2. Switching CR3 would affect the entire kernel's memory view
    // 3. A real page fault would need proper exception handling setup
    
    // Instead, we'll verify the conditions that WOULD cause a page fault:
    
    // 1. Verify PTE is not present (would cause page fault)
    page_table_entry *fault_pte = vmm_walk_page_table(&test_as, fault_test_vaddr, false);
    if (fault_pte && (*fault_pte & PTE_PRESENT)) {
        KERROR("FAIL: PTE still present - would NOT cause page fault\n");
        return -1;
    }
    KSUCCESS("PTE not present - would cause page fault on access\n");
    
    // 2. Verify translation fails (would cause page fault)
    phys_addr fault_translation = vmm_virt_to_phys(&test_as, fault_test_vaddr);
    if (fault_translation != 0) {
        kprintf("INFO: Translation returned 0x%lx (implementation specific)\n", fault_translation);
    } else {
        KSUCCESS("Translation fails - would cause page fault on access\n");
    }
    
    // 3. Verify vmm_is_mapped returns false (would cause page fault)
    if (vmm_is_mapped(&test_as, fault_test_vaddr)) {
        KERROR("FAIL: vmm_is_mapped returns true - inconsistent state\n");
        return -1;
    }
    KSUCCESS("vmm_is_mapped returns false - confirms page fault would occur\n");
    
    // 4. Test page fault simulation function (if available)
    // This would be a custom function you could implement to test page fault handling
    // without actually switching address spaces
    #ifdef TEST_PAGE_FAULT_SIMULATION
    if (test_page_fault_simulation) {
        kprintf("Testing page fault simulation...\n");
        bool fault_occurred = simulate_page_fault(&test_as, fault_test_vaddr);
        if (!fault_occurred) {
            KERROR("FAIL: Page fault simulation should have detected unmapped page\n");
            return -1;
        }
        KSUCCESS("Page fault simulation correctly detected unmapped page\n");
    }
    #endif
    
    // 5. Verify that remapping allows access again
    if (vmm_map_page(&test_as, fault_test_vaddr, fault_test_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: Failed to remap page for fault recovery test\n");
        return -1;
    }
    
    // Verify mapping is restored
    if (!vmm_is_mapped(&test_as, fault_test_vaddr)) {
        KERROR("FAIL: Page not mapped after remapping\n");
        return -1;
    }
    
    phys_addr recovered_paddr = vmm_virt_to_phys(&test_as, fault_test_vaddr);
    if (recovered_paddr != fault_test_paddr) {
        KERROR("FAIL: Translation incorrect after remapping\n");
        return -1;
    }
    
    // Verify data is still there
    uint64_t recovered_pattern = *fault_test_ptr;
    if (recovered_pattern != fault_test_pattern) {
        KERROR("FAIL: Data corrupted during unmap/remap cycle\n");
        return -1;
    }
    
    KSUCCESS("Page fault conditions verified - unmapped page would cause fault\n");
    KSUCCESS("Remapping restores access capability\n");
     
    kprintf("NOTE: Actual page fault would occur if this address space was active\n");
    kprintf("      and virtual address 0x%lx was accessed\n", fault_test_vaddr);
    kprintf("=== Page Fault Test Complete ===\n");
    
    kprintf("=== Unmap memory access tests passed! ===\n\n");
    return 0;
}

// Test error conditions
static int test_vmm_errors(void) {
    kprintf("=== VMM Error Handling Test ===\n");
    
    struct addr_space *test_as = vmm_create_address_space();
    
    // Test 1: Double mapping should fail
    virt_addr vaddr = 0x600000;
    phys_addr paddr1 = pmm_alloc_page();
    phys_addr paddr2 = pmm_alloc_page();
    // First mapping should succeed
    if (vmm_map_page(test_as, vaddr, paddr1, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: First mapping failed\n");
        return -1;
    }
    kprintf("Expecting an error...\n"); 
    // Second mapping to same virtual address should fail
    if (vmm_map_page(test_as, vaddr, paddr2, PTE_WRITABLE | PTE_USER) == 0) {
        KERROR("FAIL: Double mapping should have failed but didn't\n");
        return -1;
    }
    KSUCCESS("Double mapping correctly rejected\n");
    
    kprintf("Expecting an error...\n"); 
    // Test 2: NULL address space
    if (vmm_map_page(NULL, 0x700000, paddr2, PTE_WRITABLE | PTE_USER) == 0) {
        kprintf("FAIL: NULL address space should have failed\n");
        return -1;
    }
    KSUCCESS("NULL address space correctly rejected\n");
    
    // Test 3: Unmap error conditions
    // NULL address space
    if (vmm_unmap_range(NULL, 0x400000, PAGE_SIZE) == 0) {
        KERROR("FAIL: NULL address space unmap should have failed\n");
        return -1;
    }
    KSUCCESS("Unmap with NULL address space correctly rejected\n");
    
    // Zero size
    if (vmm_unmap_range(test_as, 0x400000, 0) == 0) {
        KERROR("FAIL: Zero size unmap should have failed\n");
        return -1;
    }
    KSUCCESS("Unmap with zero size correctly rejected\n");
    
    // Unmapping non-existent mapping should succeed (no-op)
    if (vmm_unmap_range(test_as, 0x900000, PAGE_SIZE) != 0) {
        kprintf("INFO: Unmapping non-existent page returned error (implementation choice)\n");
    } else {
        KSUCCESS("Unmapping non-existent page handled gracefully\n");
    }
    
    kprintf("=== Error handling tests passed! ===\n\n");
    return 0;
}

static int test_memory_access_safe(void) {
    kprintf("=== Safe Memory Access Test (No CR3 Switch) ===\n");
    
    struct addr_space test_as;
    test_as.pml4 = vmm_alloc_page_table();
    test_as.total_pages = 0;
    
    if (!test_as.pml4) {
        KERROR("FAIL: Could not allocate PML4\n");
        return -1;
    }
    
    phys_addr test_paddr = pmm_alloc_page();
    if (test_paddr == 0) {
        KERROR("FAIL: Could not allocate physical page\n");
        return -1;
    }
    
    virt_addr test_vaddr = 0x400000;
    
    // Write test pattern to physical memory
    uint64_t *phys_ptr = (uint64_t *)(test_paddr + get_hhdm_offset());
    uint64_t test_pattern = 0xDEADBEEFCAFEBABE;
    *phys_ptr = test_pattern;
    
    kprintf("Wrote 0x%lx to physical 0x%lx (via HHDM at %p)\n", 
            test_pattern, test_paddr, phys_ptr);
    
    // Map the page
    if (vmm_map_page(&test_as, test_vaddr, test_paddr, PTE_WRITABLE | PTE_USER) != 0) {
        KERROR("FAIL: vmm_map_page failed\n");
        return -1;
    }
    
    // Verify our translation function works correctly
    phys_addr resolved = vmm_virt_to_phys(&test_as, test_vaddr);
    if (resolved != test_paddr) {
        KERROR("FAIL: Translation wrong. Expected: 0x%lx, Got: 0x%lx\n",
                test_paddr, resolved);
        return -1;
    }
    
    // Calculate what the virtual address SHOULD map to
    page_table_entry *pte = vmm_walk_page_table(&test_as, test_vaddr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        KERROR("FAIL: PTE not present\n");
        return -1;
    }
    
    phys_addr pte_phys = PTE_ADDR(*pte);
    uint64_t *pte_virt_ptr = (uint64_t *)(pte_phys + get_hhdm_offset());
    
    kprintf("PTE points to physical 0x%lx, accessing via HHDM at %p\n", 
            pte_phys, pte_virt_ptr);
    
    // Read the same physical memory that our virtual address SHOULD point to
    uint64_t pte_read = *pte_virt_ptr;
    
    if (pte_read == test_pattern) {
        KSUCCESS("PTE correctly points to test data\n");
        KSUCCESS("Virtual 0x%lx -> Physical 0x%lx contains our test pattern 0x%lx\n",
                test_vaddr, pte_phys, test_pattern);
        KSUCCESS("Page table mapping is correct\n");
    } else {
        KERROR("FAIL: PTE points to wrong physical memory\n");
        kprintf("Expected: 0x%lx, Got: 0x%lx\n", test_pattern, pte_read);
        return -1;
    }
    kprintf("=== Safe memory access tests passed! ===\n");
    return 0;
}
*/

// Main test function to call from your kernel
void run_vmm_tests(void) {
    kprintf("Starting VMM tests...\n");
    
    if (test_vmm_basic() != 0) {
        kprintf("Basic tests failed!\n");
        return;
    }
    
    if (test_vmm_range() != 0) {
        kprintf("Range tests failed!\n");
        return;
    }
    
    if (test_vmm_unmap() != 0) {
        kprintf("Unmap tests failed!\n");
        return;
    }
    
    kprintf("\n");
    KSUCCESS("All VMM tests passed!\n");
}
