#include <kernel/gdt_init.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>

struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_check;

void kernel_main(void) {
    
    terminal_initialize(); // Set up VGA for text output  
    init_gdt(); // Set up GDT and reload segment buffers 
    init_idt(); // Set up IDT
  
    kprintf("%d", 5/0);
    
    //kprintf("hello");
    
}
