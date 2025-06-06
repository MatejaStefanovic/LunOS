#ifndef __KERNEL_MEMUTILS_H
#define __KERNEL_MEMUTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/klogging.h>
#include <kernel/limine_requests.h>
#include <kernel/halt.h>

typedef uint64_t pte_t; 
typedef uint64_t vaddr_t;
typedef uint64_t paddr_t;

static inline void set_cr3(paddr_t pml4_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}

static inline paddr_t get_cr3(void) {
    paddr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline uint64_t get_hhdm_offset(void){ 
    struct limine_hhdm_request *hhdm_request = get_hhdm_request();
    
    if(!hhdm_request || !hhdm_request->response){
        kprintf("ERROR: Cannot get hhdm from limine\nHalting");
        hcf();
    }
    
    return hhdm_request->response->offset;
}

#endif
