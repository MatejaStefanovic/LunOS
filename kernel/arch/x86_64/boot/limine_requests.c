#include <kernel/limine_requests.h>

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.


// Start and end markers for the Limine requests
__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// Framebuffer request
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Memory map request
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// HHDM request
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

// Kernel address request
__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

// Getter functions
 
struct limine_framebuffer_request* get_framebuffer_request(void) {
    return (struct limine_framebuffer_request*)&framebuffer_request;
}

struct limine_memmap_request* get_memmap_request(void) {
    return (struct limine_memmap_request*)&memmap_request;
}

struct limine_hhdm_request* get_hhdm_request(void) {
    return (struct limine_hhdm_request*)&hhdm_request;
}

struct limine_kernel_address_request* get_kernel_address_request(void){
    return (struct limine_kernel_address_request*)&kernel_address_request;
}
