#include <kernel/tty.h>
#include <kernel/klogging.h>

void kernel_main(void) {
	terminal_initialize();
	kprintf("%s", "samalamadumalama");
}
