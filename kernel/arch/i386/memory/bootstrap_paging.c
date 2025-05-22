#include <kernel/paging.h>

uint32_t pd[1024] __attribute__((aligned(4096)));
uint32_t pt[1024] __attribute__((aligned(4096)));


void fill_pt(){
    uint32_t i;
    for(i = 0; i < 1024; ++i){
        pt[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
    }
}

void fill_pd(){
    int i;
    // 0x00000002
    // 0000 0000 0000 0000 0000 0000 0000 0010
    for(i = 0; i < 1024; ++i){
        pd[i] = 0x00000002;
    }
    // Map 0x00000000 - 0x003FFFFF (first 4 MB) → identity
    pd[0] = ((uint32_t)pt) | 3;

    // Map 0xC0000000 - 0xC03FFFFF → same physical memory
    pd[768] = ((uint32_t)pt) | 3;
}

void jump_to_higher_half(void) {
    asm volatile (
        "movl %0, %%esp\n"
        "jmp *%1\n"
        :
        : "r" (0xC0100000), "r" (higher_half_main)
    );
}
void flush_tlb() {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3));
}
void init_paging(){
    fill_pt();
    fill_pd();

    loadPageDirectory(pd);
    enablePaging();

    //jump_to_higher_half();
    //pd[0] = 0x00000002;
    //flush_tlb();
}
