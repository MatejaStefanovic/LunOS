#include <kernel/gdt_init.h>
#include <kernel/tty.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>
#include <kernel/paging.h>
#include <kernel/pma.h>

void kernel_main(unsigned long addr) {
    init_gdt(); // Set up GDT and reload segment buffers 
    init_idt(); // Set up IDT
    terminal_initialize(); // Set up VGA for text output 
    
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO\n");
    kprintf("What happened in Monte Carlo happened...\n");
    kprintf("What happened in Barcelona happened...\n");
    kprintf("What happened in Madrdid happened and we are here, ");
    kprintf("we're in Rome...\n");
    
    parse_multiboot_mem_info(addr);
}
