/*
#include <kernel/gdt_init.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>
#include <kernel/paging.h>
#include <kernel/pma.h>
*/

#include <kernel/framebuffer.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>

// Set the base revision to 3
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void kernel_main() {

    if(!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }
    if(!fb_init())
        hcf();

    //init_gdt(); // Set up GDT and reload segment buffers
    //init_idt(); // Set up IDT
    
    fb_clear(0x000000);  // Clear to black
   
    terminal_initialize(0xFFFFFF, 0x0000FF);
    
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO\n");
    kprintf("What happened in Monte Carlo happened...\n");
    kprintf("What happened in Barcelona happened...\n");
    kprintf("What happened in Madrdid happened and we are here, ");
    kprintf("we're in Rome...\n");
    //parse_multiboot_mem_info(addr);
}
