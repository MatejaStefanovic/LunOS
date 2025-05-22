#include <kernel/gdt_init.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>
#include <kernel/paging.h>

void kernel_main(void) {
    init_gdt(); // Set up GDT and reload segment buffers 
    init_idt(); // Set up IDT
    init_paging();
    terminal_initialize(); // Set up VGA for text output  
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO");
    
}

void higher_half_main(){
    kprintf("What happened in Monte Carlo happened, what happened in Barcelona happened ");
    kprintf("What happened in Madrdid happened and we are here, we're in Rome...");
}
