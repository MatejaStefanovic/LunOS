#include <kernel/idt_init.h>
#include <kernel/framebuffer.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>

// Set the base revision to 3
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

void kernel_main() {

    if(!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }
    if(!fb_init())
        hcf();

    /* Set up GDT and reload segment buffers 
     * Limine now does it for us so no manual GDT 
     * setup is required */
    //init_gdt(); 


    fb_clear(0x000000);  // Clear to black
    terminal_initialize(0xFFFFFF, 0x000035);
    KSUCCESS("Terminal initialized properly\n");

    kprintf("Initializing Interrupt table...\n");
    init_idt(); // Set up IDT
    KSUCCESS("Interrupt setup was successfull\n");  
    
    kprintf("Setting up buddy allocator ...\n");
    buddy_allocator_init();
    KSUCCESS("Buddy allocator initialized properly\n");
    
    print_buddy_arena(0);
   
    void *ptr = kmalloc(4096*sizeof(struct idt_entry_t));
    kprintf("\n");
    print_buddy_arena(0);
    kprintf("\n");
    kfree(ptr);
    print_buddy_arena(0);
    hcf();

}

/* 
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO\n");
    kprintf("What happened in Monte Carlo happened...\n");
    kprintf("What happened in Barcelona happened...\n");
    kprintf("What happened in Madrdid happened and we are here, ");
    kprintf("we're in Rome...\n\n");
*/
