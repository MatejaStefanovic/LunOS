#ifndef __KERNEL_PAGING_H
#define __KERNEL_PAGING_H

#include <stdint.h>

struct pde_t {
    uint8_t present              : 1;  // Bit 0 is page in physical memory?
    uint8_t r_w                  : 1;  // Bit 1 if it is set page is read+write otherwise read only
    uint8_t u_s                  : 1;  // Bit 2 sets privilege level - if set accessed by all otherwise read only
    uint8_t pwt                  : 1;  // Bit 3 page write through caching enable/disable
    uint8_t pcd                  : 1;  // Bit 4 page cache disabled if set otherwise it is cached
    uint8_t accessed             : 1;  // Bit 5 was PDE read during VAddr translation
    uint8_t reserved             : 1;  // Bit 6 (reserved for 4KB pages)
    uint8_t ps                   : 1;  // Bit 7 is 0 for 4KiB pages, 1 for 4MiB pages (we use 0)
    uint8_t avl_for_os           : 4;  // Bits 8-11, OS can store info on these bits
    uint32_t pt_addr             : 20; // Bits 12-31 (address of page table >> 12)
} __attribute__((packed));

struct pte_t{
    uint8_t present              : 1;  // Bit 0 is page in physical memory?
    uint8_t r_w                  : 1;  // Bit 1 if it is set page is read+write otherwise read only
    uint8_t u_s                  : 1;  // Bit 2 sets privilege level - if set accessed by all otherwise read only
    uint8_t pwt                  : 1;  // Bit 3 page write through caching enable/disable
    uint8_t pcd                  : 1;  // Bit 4 page cache disabled if set otherwise it is cached
    uint8_t accessed             : 1;  // Bit 5 was PDE read during VAddr translation
    uint8_t dirty                : 1;  // Bit 6 was PDE read during VAddr translation
    uint8_t pat                  : 1;  // Bit 7 was PDE read during VAddr translation
    uint8_t global               : 1;  // Bit 8 was PDE read during VAddr translation
    uint8_t avl_for_os           : 3;  // Bits 9-11, OS can store info on these bits
    uint32_t pf_addr             : 20; // Bits 12-31 4KiB page frame address  
} __attribute__((packed));

extern void loadPageDirectory(uint32_t *);
extern void enablePaging();
void fill_pd();
void fill_pt();
void init_paging();

extern void higher_half_main(void);
void jump_to_higher_half();
#endif
