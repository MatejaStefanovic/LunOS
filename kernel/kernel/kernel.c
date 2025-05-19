#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/gdt_init.h>

void kernel_main(void) {
    init_gdt(); // Set up GDT and reload segment buffers 
    terminal_initialize(); // Set up VGA for text output  
	kprintf("Successfuly set up Global Descriptor Table");
}
