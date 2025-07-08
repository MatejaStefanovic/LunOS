#ifndef __KERNEL_PHYSICAL_MEM_MANAGER_H
#define __KERNEL_PHYSICAL_MEM_MANAGER_H

#include <kernel/buddy_allocator.h>
#include <kernel/slab_allocator.h>
#include <kernel/compiler.h>
#include <stdbool.h>

#define ALLOC_MAGIC 0xDEADBEEFCAFEBABE
// We detect double free with this, if you kmalloc and kfree twice if we don't stop it
// it could corrupt the allocator
#define FREED_PATTERN 0xDEADDEADDEADDEAD 

struct alloc_header {
    uint64_t magic;
    uint32_t size;
    uint8_t order;
    bool is_slab;
} _aligned(8);

void *kmalloc(size_t size);
void kfree(void* ptr);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t phys);

#endif
