#include <kernel/gdt_init.h>
#include <kernel/idt_init.h>
#include <kernel/framebuffer.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/apic.h>
#include <kernel/timer.h>
#include <kernel/smp.h>
#include <kernel/scheduler.h>

#include <tests/malloc_tests.h>
#include <tests/vmm_tests.h>

// Set the base revision to 3
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

void _start(void);
void _start(void) {
    if(!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }
    if(fb_init() != 0)
        hcf();

    fb_clear(0x000000);  // Clear to black
    terminal_initialize(0xFFFFFF, 0x000035);
    KSUCCESS("Terminal initialized properly\n");

    init_gdt();
    init_idt();

    buddy_allocator_init();
    slab_allocator_init();

    if(vmm_init() != 0)
        KERROR("Failed to initialize virtual memory manager\n");
    else
        KSUCCESS("Virtual memory manager initialized properly\n");
   
    apic_global_init();
    apic_timer_register_handler();
    reload_idt();
    
    scheduler_init();
 
    smp_init(); 
    
    while(1)
        __asm__ __volatile__("pause");
}

