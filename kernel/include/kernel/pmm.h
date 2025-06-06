#ifndef __KERNEL_PHYSICAL_MEM_MANAGER_H
#define __KERNEL_PHYSICAL_MEM_MANAGER_H

#include <kernel/buddy_allocator.h>

#define HEAP_MAGIC 0xDEADBEEF
struct heap_header {
    uint32_t magic;
    uint32_t size;
    uint8_t order;
}__attribute__((aligned(8)));

void *kmalloc(size_t size);
void kfree(void* ptr);

#endif
