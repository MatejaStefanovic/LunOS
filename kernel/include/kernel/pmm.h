#ifndef __KERNEL_PHYSICAL_MEM_MANAGER_H
#define __KERNEL_PHYSICAL_MEM_MANAGER_H

#include <kernel/buddy_allocator.h>
#include <kernel/slab_allocator.h>

#define ALLOC_MAGIC 0xDEADBEEF

struct alloc_header {
    uint32_t magic;
    uint32_t size;
    uint8_t order;
}__attribute__((aligned(8)));

void *kmalloc(size_t size);
void kfree(void* ptr);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t phys);

#endif
