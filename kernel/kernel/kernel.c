#include <kernel/gdt_init.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>
#include <stdio.h>


void kernel_main(void) {
    init_gdt(); // Set up GDT and reload segment buffers 
    init_idt(); // Set up IDT
    terminal_initialize(); // Set up VGA for text output  
    kprintf("%d", 1/0); 
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO");
    
}
