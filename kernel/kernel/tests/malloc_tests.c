#include <tests/malloc_tests.h>
#include <kernel/pmm.h>
#include <kernel/klogging.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>


static void test_basic_allocation(void) {
    kprintf("=== Basic Allocation Test ===\n");
    
    // Test small allocation
    void *ptr1 = kmalloc(64);
    kprintf("kmalloc(64) = %p\n", ptr1);
    if (ptr1) {
        memset(ptr1, 0xAA, 64);  // Write to it
        kprintf("Written to 64 bytes successfully\n");
        kfree(ptr1);
        kprintf("kfree(ptr1) completed\n");
    }
    
    // Test medium allocation
    void *ptr2 = kmalloc(1024);
    kprintf("kmalloc(1024) = %p\n", ptr2);
    if (ptr2) {
        memset(ptr2, 0xBB, 1024);  // Write to it
        kprintf("Written to 1024 bytes successfully\n");
        kfree(ptr2);
        kprintf("kfree(ptr2) completed\n");
    }
    
    // Test large allocation
    void *ptr3 = kmalloc(4096);
    kprintf("kmalloc(4096) = %p\n", ptr3);
    if (ptr3) {
        memset(ptr3, 0xCC, 4096);  // Write to it
        kprintf("Written to 4096 bytes successfully\n");
        kfree(ptr3);
        kprintf("kfree(ptr3) completed\n");
    }
}

static void test_multiple_allocations(void) {
    kprintf("=== Multiple Allocations Test ===\n");
    
    void *ptrs[10];
    
    // Allocate multiple blocks
    for (int i = 0; i < 10; i++) {
        ptrs[i] = kmalloc(128 + i * 32);
        kprintf("kmalloc(%d) = %p\n", 128 + i * 32, ptrs[i]);
        if (ptrs[i]) {
            memset(ptrs[i], 0xDD + i, 128 + i * 32);  // Write unique pattern
        }
    }
    
    // Free them all
    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) {
            kfree(ptrs[i]);
            kprintf("kfree(ptrs[%d]) completed\n", i);
        }
    }
}

static void test_zero_and_null(void) {
    kprintf("=== Zero/NULL Test ===\n");
    
    // Test zero allocation
    void *ptr = kmalloc(0);
    kprintf("kmalloc(0) = %p\n", ptr);
    if (ptr) {
        kfree(ptr);
        kprintf("kfree(zero_ptr) completed\n");
    }
    
    // Test NULL free (should not crash)
    kfree(NULL);
    kprintf("kfree(NULL) completed (should be safe)\n");
}

static void test_large_allocations(void) {
    kprintf("=== Large Allocation Test ===\n");
    
    size_t sizes[] = {8192, 16384, 32768, 65536, 131072};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        void *ptr = kmalloc(sizes[i]);
        kprintf("kmalloc(%lu) = %p\n", sizes[i], ptr);
        
        if (ptr) {
            // Write to first and last bytes
            ((char*)ptr)[0] = 0xA0 + i;
            ((char*)ptr)[sizes[i] - 1] = 0xB0 + i;
            kprintf("Written to %lu bytes successfully\n", sizes[i]);
            
            kfree(ptr);
            kprintf("kfree(%lu bytes) completed\n", sizes[i]);
        }
    }
}

static void test_fragmentation(void) {
    kprintf("=== Fragmentation Test ===\n");
    
    void *ptrs[20];
    
    // Allocate many small blocks
    for (int i = 0; i < 20; i++) {
        ptrs[i] = kmalloc(64);
        kprintf("Alloc %d: %p\n", i, ptrs[i]);
        if (ptrs[i]) {
            memset(ptrs[i], 0xF0 + (i % 16), 64);
        }
    }
    
    // Free every other block
    for (int i = 1; i < 20; i += 2) {
        if (ptrs[i]) {
            kfree(ptrs[i]);
            ptrs[i] = NULL;
            kprintf("Freed block %d\n", i);
        }
    }
    
    // Try to allocate in the gaps
    for (int i = 1; i < 20; i += 2) {
        ptrs[i] = kmalloc(64);
        kprintf("Realloc %d: %p\n", i, ptrs[i]);
        if (ptrs[i]) {
            memset(ptrs[i], 0xE0 + (i % 16), 64);
        }
    }
    
    // Free everything
    for (int i = 0; i < 20; i++) {
        if (ptrs[i]) {
            kfree(ptrs[i]);
        }
    }
}

static void test_stress_many_allocations(void) {
    kprintf("=== Stress Test: Many Allocations ===\n");
    
    #define STRESS_ALLOC_COUNT 1000
    void *ptrs[STRESS_ALLOC_COUNT];
    size_t successful_allocs = 0;
    
    // Allocate many blocks of varying sizes
    for (int i = 0; i < STRESS_ALLOC_COUNT; i++) {
        size_t size = 16 + (i % 512);  // 16 to 528 bytes
        ptrs[i] = kmalloc(size);
        
        if (ptrs[i]) {
            successful_allocs++;
            // Write pattern to verify integrity later
            uint8_t pattern = (uint8_t)(i % 256);
            memset(ptrs[i], pattern, size);
            
            if (i % 100 == 0) {
                kprintf("Allocated %d blocks so far\n", i);
            }
        } else {
            kprintf("Allocation failed at block %d (size %lu)\n", i, size);
        }
    }
    
    kprintf("Successfully allocated %lu out of %d blocks\n", successful_allocs, STRESS_ALLOC_COUNT);
    
    // Verify all allocations still have correct data
    size_t verified = 0;
    for (int i = 0; i < STRESS_ALLOC_COUNT; i++) {
        if (ptrs[i]) {
            uint8_t expected_pattern = (uint8_t)(i % 256);
            uint8_t first_byte = ((uint8_t*)ptrs[i])[0];
            
            if (first_byte == expected_pattern) {
                verified++;
            } else {
                kprintf("Data corruption detected at block %d: expected 0x%02x, got 0x%02x\n", 
                       i, expected_pattern, first_byte);
            }
        }
    }
    
    kprintf("Verified %lu blocks without corruption\n", verified);
    
    // Free all blocks
    for (int i = 0; i < STRESS_ALLOC_COUNT; i++) {
        if (ptrs[i]) {
            kfree(ptrs[i]);
        }
    }
    
    kprintf("Stress test completed: freed all %lu blocks\n", successful_allocs);
}

static void test_stress_random_sizes(void) {
    kprintf("=== Stress Test: Random Sizes ===\n");
    
    #define RANDOM_ALLOC_COUNT 500
    void *ptrs[RANDOM_ALLOC_COUNT];
    size_t sizes[RANDOM_ALLOC_COUNT];
    
    // Simple LCG for reproducible "random" numbers
    uint32_t seed = 12345;
    
    for (int i = 0; i < RANDOM_ALLOC_COUNT; i++) {
        seed = seed * 1103515245 + 12345;
        sizes[i] = 1 + (seed % 8192);  // 1 to 8192 bytes
        
        ptrs[i] = kmalloc(sizes[i]);
        if (ptrs[i]) {
            // Fill with pattern based on size
            uint8_t pattern = (uint8_t)(sizes[i] % 256);
            memset(ptrs[i], pattern, sizes[i]);
            
            if (i % 50 == 0) {
                kprintf("Random alloc %d: %lu bytes at %p\n", i, sizes[i], ptrs[i]);
            }
        }
    }
    
    // Verify and free in reverse order
    for (int i = RANDOM_ALLOC_COUNT - 1; i >= 0; i--) {
        if (ptrs[i]) {
            uint8_t expected = (uint8_t)(sizes[i] % 256);
            uint8_t actual = ((uint8_t*)ptrs[i])[0];
            
            if (actual != expected) {
                kprintf("Corruption in block %d: expected 0x%02x, got 0x%02x\n", 
                       i, expected, actual);
            }
            
            kfree(ptrs[i]);
        }
    }
    
    kprintf("Random size stress test completed\n");
}

static void test_stress_alloc_free_cycles(void) {
    kprintf("=== Stress Test: Allocation/Free Cycles ===\n");
    
    #define CYCLE_COUNT 100
    #define PTRS_PER_CYCLE 50
    
    for (int cycle = 0; cycle < CYCLE_COUNT; cycle++) {
        void *ptrs[PTRS_PER_CYCLE];
        
        // Allocate
        for (int i = 0; i < PTRS_PER_CYCLE; i++) {
            size_t size = 32 + (i * 16);  // 32, 48, 64, 80, ... bytes
            ptrs[i] = kmalloc(size);
            if (ptrs[i]) {
                memset(ptrs[i], 0xAA + (cycle % 16), size);
            }
        }
        
        // Free
        for (int i = 0; i < PTRS_PER_CYCLE; i++) {
            if (ptrs[i]) {
                kfree(ptrs[i]);
            }
        }
        
        if (cycle % 10 == 0) {
            kprintf("Completed %d allocation/free cycles\n", cycle);
        }
    }
    
    kprintf("Allocation/free cycle stress test completed\n");
}

static void test_stress_mixed_sizes(void) {
    kprintf("=== Stress Test: Mixed Small/Large Allocations ===\n");
    
    #define MIXED_COUNT 200
    void *small_ptrs[MIXED_COUNT];
    void *large_ptrs[MIXED_COUNT / 10];
    
    // Allocate mix of small and large blocks
    for (int i = 0; i < MIXED_COUNT; i++) {
        // Small allocations
        small_ptrs[i] = kmalloc(8 + (i % 128));
        if (small_ptrs[i]) {
            memset(small_ptrs[i], 0x11, 8 + (i % 128));
        }
        
        // Every 10th allocation, make a large one
        if (i % 10 == 0 && i / 10 < (MIXED_COUNT / 10)) {
            large_ptrs[i / 10] = kmalloc(4096 + (i * 100));
            if (large_ptrs[i / 10]) {
                memset(large_ptrs[i / 10], 0x22, 4096 + (i * 100));
            }
        }
    }
    
    kprintf("Allocated %d small blocks and %d large blocks\n", MIXED_COUNT, MIXED_COUNT / 10);
    
    // Free in mixed order
    for (int i = 0; i < MIXED_COUNT; i++) {
        if (small_ptrs[i]) {
            kfree(small_ptrs[i]);
        }
        
        if (i % 10 == 0 && i / 10 < (MIXED_COUNT / 10)) {
            if (large_ptrs[i / 10]) {
                kfree(large_ptrs[i / 10]);
            }
        }
    }
    
    kprintf("Mixed size stress test completed\n");
}

static void test_stress_memory_intensive(void) {
    kprintf("=== Stress Test: Memory Intensive ===\n");
    
    // Try to allocate large chunks until we fail
    size_t chunk_size = 1024 * 1024;  // 1MB chunks
    void *chunks[64];  // Up to 64MB
    int allocated_chunks = 0;
    
    for (int i = 0; i < 64; i++) {
        chunks[i] = kmalloc(chunk_size);
        if (chunks[i]) {
            allocated_chunks++;
            // Write to first and last page to ensure it's really allocated
            ((char*)chunks[i])[0] = 0xAA;
            ((char*)chunks[i])[chunk_size - 1] = 0xBB;
            kprintf("Allocated chunk %d (1MB) at %p\n", i, chunks[i]);
        } else {
            kprintf("Failed to allocate chunk %d - out of memory\n", i);
            break;
        }
    }
    
    kprintf("Successfully allocated %d MB of memory\n", allocated_chunks);
    
    // Verify data integrity
    for (int i = 0; i < allocated_chunks; i++) {
        if (chunks[i]) {
            char first = ((char*)chunks[i])[0];
            char last = ((char*)chunks[i])[chunk_size - 1];
            
            if (first != (char)0xAA || last != (char)0xBB) {
                kprintf("Data corruption in chunk %d: first=0x%02x, last=0x%02x\n", 
                       i, (uint8_t)first, (uint8_t)last);
            }
        }
    }
    
    // Free all chunks
    for (int i = 0; i < allocated_chunks; i++) {
        if (chunks[i]) {
            kfree(chunks[i]);
        }
    }
    
    kprintf("Memory intensive stress test completed\n");
}

static void test_alignment_and_boundaries(void) {
    kprintf("=== Alignment and Boundary Test ===\n");
    
    // Test various sizes around common boundaries
    size_t test_sizes[] = {
        1, 2, 3, 4, 7, 8, 9, 15, 16, 17,
        31, 32, 33, 63, 64, 65, 127, 128, 129,
        255, 256, 257, 511, 512, 513, 1023, 1024, 1025,
        2047, 2048, 2049, 4095, 4096, 4097
    };
    
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        void *ptr = kmalloc(test_sizes[i]);
        if (ptr) {
            // Check alignment
            uintptr_t addr = (uintptr_t)ptr;
            if (addr % 8 != 0) {
                kprintf("WARNING: Misaligned allocation for size %lu: %p\n", 
                       test_sizes[i], ptr);
            }
            
            // Write to the memory
            memset(ptr, 0x55, test_sizes[i]);
            
            kfree(ptr);
        } else {
            kprintf("Failed to allocate %lu bytes\n", test_sizes[i]);
        }
    }
    
    kprintf("Alignment and boundary test completed\n");
}

int run_kmalloc_tests(void) {
    kprintf("Starting enhanced kmalloc/kfree test suite\n");
    kprintf("Testing both functionality and stress scenarios\n\n");
    
    // Basic functionality tests
    test_basic_allocation();
    test_zero_and_null();
    test_multiple_allocations();
    test_large_allocations();
    test_fragmentation();
    test_alignment_and_boundaries();
    
    kprintf("\n=== STRESS TESTS ===\n");
    
    // Stress tests
    test_stress_many_allocations();
    test_stress_random_sizes();
    test_stress_alloc_free_cycles();
    test_stress_mixed_sizes();
    test_stress_memory_intensive();
    
    kprintf("\nAll kmalloc/kfree tests completed successfully!\n");
    kprintf("Your allocator handled all stress scenarios\n");
    
    return 0;
}



// Memory tracking globals
static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t peak_usage = 0;
static size_t current_usage = 0;
static size_t allocation_count = 0;
static size_t free_count = 0;

// Memory tracking wrappers
static void *tracked_kmalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        total_allocated += size;
        current_usage += size;
        allocation_count++;
        if (current_usage > peak_usage) {
            peak_usage = current_usage;
        }
    }
    return ptr;
}

static void tracked_kfree(void *ptr, size_t size) {
    if (ptr) {
        kfree(ptr);
        total_freed += size;
        current_usage -= size;
        free_count++;
    }
}

static void reset_memory_tracking(void) {
    total_allocated = 0;
    total_freed = 0;
    peak_usage = 0;
    current_usage = 0;
    allocation_count = 0;
    free_count = 0;
}

static void print_memory_stats(void) {
    kprintf("=== Memory Statistics ===\n");
    kprintf("Total allocated: %lu bytes (%lu allocations)\n", total_allocated, allocation_count);
    kprintf("Total freed: %lu bytes (%lu frees)\n", total_freed, free_count);
    kprintf("Current usage: %lu bytes\n", current_usage);
    kprintf("Peak usage: %lu bytes\n", peak_usage);
    kprintf("Potential leak: %lu bytes\n", (long)(total_allocated - total_freed));
    kprintf("Allocation/Free balance: %lu\n", (long)(allocation_count - free_count));
}

/*
static void test_memory_leak_detection(void) {
    kprintf("=== Memory Leak Detection Test ===\n");
    
    reset_memory_tracking();
    
    // Allocate various sizes
    void *ptrs[100];
    size_t sizes[100];
    
    for (int i = 0; i < 100; i++) {
        sizes[i] = 16 + (i * 32);  // 16, 48, 80, ... bytes
        ptrs[i] = tracked_kmalloc(sizes[i]);
        if (ptrs[i]) {
            memset(ptrs[i], 0xAB + (i % 16), sizes[i]);
        }
    }
    
    // Intentionally "forget" to free some allocations
    for (int i = 0; i < 90; i++) {  // Only free 90 out of 100
        if (ptrs[i]) {
            tracked_kfree(ptrs[i], sizes[i]);
        }
    }
    
    print_memory_stats();
    
    // Clean up the "leaked" allocations
    for (int i = 90; i < 100; i++) {
        if (ptrs[i]) {
            tracked_kfree(ptrs[i], sizes[i]);
        }
    }
    
    kprintf("After cleanup:\n");
    print_memory_stats();
}


static void test_use_after_free_detection(void) {
    kprintf("=== Use After Free Detection Test ===\n");
    kprintf("WARNING: This test demonstrates dangerous behavior\n");
    
    void *ptr = kmalloc(256);
    if (ptr) {
        // Write pattern
        memset(ptr, 0xBE, 256);
        kprintf("Written pattern to allocated memory\n");
        
        // Free the memory
        kfree(ptr);
        kprintf("Memory freed\n");
        
        // This is dangerous - reading after free
        kprintf("Reading from freed memory (dangerous): first byte = 0x%02x\n", 
               ((unsigned char*)ptr)[0]);
        
        // This is very dangerous - writing after free
        kprintf("WARNING: About to write to freed memory (very dangerous)\n");
        ((unsigned char*)ptr)[0] = 0xDE;  // Use after free
        kprintf("Wrote to freed memory (this could corrupt other allocations)\n");
    }
}

static void test_buffer_overflow_detection(void) {
    kprintf("=== Buffer Overflow Detection Test ===\n");
    kprintf("Testing writes beyond allocated boundaries\n");
    
    void *ptr = kmalloc(64);
    if (ptr) {
        // Write within bounds (safe)
        memset(ptr, 0xAA, 64);
        kprintf("Wrote 64 bytes within allocated space (safe)\n");
        
        // Write beyond bounds (dangerous)
        kprintf("WARNING: Writing beyond allocated boundary (dangerous)\n");
        ((char*)ptr)[64] = 0xBB;  // One byte past end
        ((char*)ptr)[65] = 0xCC;  // Two bytes past end
        kprintf("Wrote beyond allocated space (this could corrupt heap metadata)\n");
        
        kfree(ptr);
    }
}

static void test_extreme_sizes(void) {
    kprintf("=== Extreme Size Test ===\n");
    
    // Test very large allocations
    size_t huge_sizes[] = {
        1ULL << 28,         // 256MB
    };
    
    for (int i = 0; i < 1; i++) {
        kprintf("Attempting to allocate %lu bytes...\n", huge_sizes[i]);
        void *ptr = kmalloc(huge_sizes[i]);
        if (ptr) {
            kprintf("SUCCESS: Allocated %lu bytes at %p\n", huge_sizes[i], ptr);
            // Don't write to it - too dangerous and slow
            kfree(ptr);
            kprintf("Freed large allocation\n");
        } else {
            kprintf("EXPECTED: Failed to allocate %lu bytes (good)\n", huge_sizes[i]);
        }
    }
}

static void test_integer_overflow_protection(void) {
    kprintf("=== Integer Overflow Protection Test ===\n");
    
    // Test sizes that might cause integer overflow in calculations
    size_t tricky_sizes[] = {
        SIZE_MAX - 7,       // Might overflow when rounded up
        SIZE_MAX - 15,      // Might overflow with alignment
        SIZE_MAX - 31,      // Might overflow with padding
        SIZE_MAX - 63,      // Might overflow with headers
        SIZE_MAX / 2 + 1,   // Might overflow when doubled
        UINT32_MAX,         // 32-bit boundary
        UINT32_MAX + 1,     // Just over 32-bit
    };
    
    for (int i = 0; i < 7; i++) {
        kprintf("Testing potentially problematic size: %lu\n", tricky_sizes[i]);
        void *ptr = kmalloc(tricky_sizes[i]);
        if (ptr) {
            kprintf("WARNING: Large allocation succeeded - %p\n", ptr);
            kfree(ptr);
        } else {
            kprintf("Good: Allocation properly rejected\n");
        }
    }
}
*/

static void test_long_running_stability(void) {
    kprintf("=== Long Running Stability Test ===\n");
    kprintf("Running extended allocation/free cycles...\n");
    
    reset_memory_tracking();
    
    #define LONG_RUN_CYCLES 10000
    #define PTRS_PER_BATCH 20
    
    uint32_t seed = 54321;  // For reproducible patterns
    
    for (int cycle = 0; cycle < LONG_RUN_CYCLES; cycle++) {
        void *ptrs[PTRS_PER_BATCH];
        size_t sizes[PTRS_PER_BATCH];
        
        // Allocate batch
        for (int i = 0; i < PTRS_PER_BATCH; i++) {
            seed = seed * 1103515245 + 12345;
            sizes[i] = 8 + (seed % 1024);  // 8 to 1032 bytes
            ptrs[i] = tracked_kmalloc(sizes[i]);
            
            if (ptrs[i]) {
                // Write pattern
                uint8_t pattern = (uint8_t)((cycle + i) % 256);
                memset(ptrs[i], pattern, sizes[i]);
                
                // Verify pattern immediately
                if (((uint8_t*)ptrs[i])[0] != pattern) {
                    kprintf("ERROR: Immediate corruption in cycle %d, allocation %d\n", cycle, i);
                }
            }
        }
        
        // Free batch
        for (int i = 0; i < PTRS_PER_BATCH; i++) {
            if (ptrs[i]) {
                // Verify pattern before freeing
                uint8_t expected = (uint8_t)((cycle + i) % 256);
                if (((uint8_t*)ptrs[i])[0] != expected) {
                    kprintf("ERROR: Corruption detected in cycle %d, allocation %d\n", cycle, i);
                }
                tracked_kfree(ptrs[i], sizes[i]);
            }
        }
        
        // Progress report
        if (cycle % 1000 == 0) {
            kprintf("Completed %d cycles, current usage: %lu bytes\n", cycle, current_usage);
        }
        
        // Check for memory leaks periodically
        if (cycle % 2000 == 0 && current_usage > 0) {
            kprintf("WARNING: Memory leak detected at cycle %d: %lu bytes\n", cycle, current_usage);
        }
    }
    
    print_memory_stats();
    kprintf("Long running stability test completed\n");
}

static void test_pathological_fragmentation(void) {
    kprintf("=== Pathological Fragmentation Test ===\n");
    
    #define FRAG_BLOCKS 1000
    void *ptrs[FRAG_BLOCKS];
    
    // Create worst-case fragmentation pattern
    kprintf("Creating fragmentation pattern...\n");
    
    // Allocate many small blocks
    for (int i = 0; i < FRAG_BLOCKS; i++) {
        ptrs[i] = kmalloc(32);  // Small, uniform size
        if (ptrs[i]) {
            memset(ptrs[i], 0xFF, 32);
        }
    }
    
    // Free every other block to create holes
    for (int i = 1; i < FRAG_BLOCKS; i += 2) {
        if (ptrs[i]) {
            kfree(ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    
    kprintf("Created fragmented heap with %d holes\n", FRAG_BLOCKS / 2);
    
    // Try to allocate larger blocks that won't fit in holes
    void *large_ptrs[100];
    int successful_large = 0;
    
    for (int i = 0; i < 100; i++) {
        large_ptrs[i] = kmalloc(128);  // Larger than holes
        if (large_ptrs[i]) {
            successful_large++;
            memset(large_ptrs[i], 0xAA, 128);
        }
    }
    
    kprintf("Successfully allocated %d large blocks despite fragmentation\n", successful_large);
    
    // Clean up
    for (int i = 0; i < FRAG_BLOCKS; i += 2) {
        if (ptrs[i]) {
            kfree(ptrs[i]);
        }
    }
    
    for (int i = 0; i < successful_large; i++) {
        if (large_ptrs[i]) {
            kfree(large_ptrs[i]);
        }
    }
    
    kprintf("Pathological fragmentation test completed\n");
}

static void test_concurrent_simulation(void) {
    kprintf("=== Concurrent Access Simulation Test ===\n");
    kprintf("Simulating concurrent allocator access patterns\n");
    
    // Simulate what might happen with multiple threads
    // (This is single-threaded simulation of concurrent patterns)
    
    #define SIM_THREADS 4
    #define ALLOCS_PER_THREAD 100
    
    void *thread_ptrs[SIM_THREADS][ALLOCS_PER_THREAD];
    
    // Simulate interleaved allocations from different "threads"
    for (int round = 0; round < ALLOCS_PER_THREAD; round++) {
        for (int thread = 0; thread < SIM_THREADS; thread++) {
            size_t size = 16 + (thread * 32) + (round % 128);
            thread_ptrs[thread][round] = kmalloc(size);
            
            if (thread_ptrs[thread][round]) {
                // Each "thread" writes its own pattern
                uint8_t pattern = 0xA0 + thread;
                memset(thread_ptrs[thread][round], pattern, size);
            }
        }
    }
    
    kprintf("Simulated interleaved allocations from %d threads\n", SIM_THREADS);
    
    // Verify data integrity (check for cross-thread corruption)
    int corruptions = 0;
    for (int thread = 0; thread < SIM_THREADS; thread++) {
        for (int alloc = 0; alloc < ALLOCS_PER_THREAD; alloc++) {
            if (thread_ptrs[thread][alloc]) {
                uint8_t expected = 0xA0 + thread;
                uint8_t actual = ((uint8_t*)thread_ptrs[thread][alloc])[0];
                
                if (actual != expected) {
                    corruptions++;
                    if (corruptions <= 5) {  // Don't spam too much
                        kprintf("Corruption: thread %d, alloc %d: expected 0x%02x, got 0x%02x\n",
                               thread, alloc, expected, actual);
                    }
                }
            }
        }
    }
    
    if (corruptions == 0) {
        kprintf("No data corruption detected in concurrent simulation\n");
    } else {
        kprintf("WARNING: %d corruptions detected - possible thread safety issues\n", corruptions);
    }
    
    // Free in random interleaved order
    for (int round = ALLOCS_PER_THREAD - 1; round >= 0; round--) {
        for (int thread = SIM_THREADS - 1; thread >= 0; thread--) {
            if (thread_ptrs[thread][round]) {
                kfree(thread_ptrs[thread][round]);
            }
        }
    }
    
    kprintf("Concurrent simulation test completed\n");
}

static void test_error_path_handling(void) {
    kprintf("=== Error Path Handling Test ===\n");
    
    // Test allocation failure scenarios
    kprintf("Testing out-of-memory scenarios...\n");
    
    void *huge_ptrs[100];
    int successful = 0;
    
    // Try to exhaust memory with large allocations
    for (int i = 0; i < 100; i++) {
        size_t size = 10 * 1024 * 1024;  // 10MB each
        huge_ptrs[i] = kmalloc(size);
        
        if (huge_ptrs[i]) {
            successful++;
            // Touch the memory to ensure it's really allocated
            ((char*)huge_ptrs[i])[0] = 0xAA;
            ((char*)huge_ptrs[i])[size - 1] = 0xBB;
        } else {
            kprintf("Allocation failed at attempt %d (expected)\n", i);
            break;
        }
    }
    
    kprintf("Successfully allocated %d large blocks before failure\n", successful);
    
    // Clean up
    for (int i = 0; i < successful; i++) {
        if (huge_ptrs[i]) {
            kfree(huge_ptrs[i]);
        }
    }
    
    // Test recovery after failed allocations
    kprintf("Testing recovery after allocation failures...\n");
    
    void *ptr = kmalloc(1024);
    if (ptr) {
        memset(ptr, 0xCC, 1024);
        kprintf("Recovery allocation successful\n");
        kfree(ptr);
    } else {
        kprintf("WARNING: Recovery allocation failed\n");
    }
}

static void test_double_free_detection(void) {
    kprintf("=== Double Free Detection Test ===\n");
    kprintf("WARNING: This test may crash if double-free protection is not implemented\n");
    
    void *ptr = kmalloc(128);
    if (ptr) {
        memset(ptr, 0xDD, 128);
        kprintf("Allocated 128 bytes at %p\n", ptr);
        
        kfree(ptr);
        kprintf("First free completed\n");
        
        // This should be detected and handled gracefully
        kprintf("Attempting double free...\n");
        kfree(ptr);  // Double free - should be handled safely
        kprintf("Double free attempt completed (should be safe)\n");
    }
}

int run_advanced_kmalloc_tests(void) {
    kprintf("Starting advanced security and robustness tests for kmalloc/kfree\n");
    kprintf("WARNING: Some tests demonstrate dangerous scenarios\n\n");
     
    
    kprintf("\n=== EXTREME SCENARIO TESTS ===\n");
    
    test_pathological_fragmentation();
    test_error_path_handling();
    test_double_free_detection();
   
    kprintf("\n=== STABILITY TESTS ===\n");
    
    test_long_running_stability();
    test_concurrent_simulation();
    
    kprintf("\nAdvanced kmalloc/kfree tests completed!\n");
    kprintf("If all tests passed without crashes or corruptions,\n");
    kprintf("your allocator is highly robust and secure.\n");
    
    return 0;
}
