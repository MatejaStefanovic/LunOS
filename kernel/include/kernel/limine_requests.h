#ifndef BOOT_LIMINE_REQUESTS_H
#define BOOT_LIMINE_REQUESTS_H

#include <kernel/limine.h>

struct limine_framebuffer_request* get_framebuffer_request(void);
struct limine_memmap_request* get_memmap_request(void);
struct limine_hhdm_request* get_hhdm_request(void);
struct limine_kernel_address_request* get_kernel_address_request(void);
struct limine_smp_request* get_smp_request(void);

#endif

