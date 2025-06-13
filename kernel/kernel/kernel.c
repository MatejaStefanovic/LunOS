#include <kernel/idt_init.h>
#include <kernel/framebuffer.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>

#include <tests/malloc_tests.h>
#include <tests/vmm_tests.h>

// Set the base revision to 3
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

void ap_entry_point(struct limine_smp_info *cpu_info) {
    for(volatile int i = 0; i < (cpu_info->lapic_id * 1000000); i++);
    kprintf("CPU %u online\n", cpu_info->lapic_id);
    
    hcf();
}

// SMP initialization
void smp_init(void) {
    struct limine_smp_request *mp_request = get_smp_request();
    if (mp_request->response == NULL) {
        kprintf("MP not available, running single-core\n");
        return;
    }
    
    struct limine_smp_response *mp_response = mp_request->response;
    KSUCCESS("Found %lu CPUs\n", mp_response->cpu_count);
    
    for (uint64_t i = 0; i < mp_response->cpu_count; i++) {
        struct limine_smp_info *cpu = mp_response->cpus[i];
        
        if (cpu->lapic_id == mp_response->bsp_lapic_id) {
            continue; // Skip BSP
        }
        
        kprintf("Starting CPU %lu (LAPIC ID: %u)\n", i, cpu->lapic_id);
        cpu->goto_address = ap_entry_point;
    }
}

void _start() {

    if(!LIMINE_BASE_REVISION_SUPPORTED) {
        hcf();
    }
    if(fb_init() != 0)
        hcf();

    /* Set up GDT and reload segment buffers 
     * Limine now does it for us so no manual GDT 
     * setup is required */
    //init_gdt(); 

    fb_clear(0x000000);  // Clear to black
    terminal_initialize(0xFFFFFF, 0x000035);
    KSUCCESS("Terminal initialized properly\n");
    
    init_idt(); // Set up IDT
    
    buddy_allocator_init();
    
    slab_allocator_init();

    if(vmm_init() != 0)
        KERROR("Failed to initialize virtual memory manager\n");
    else
        KSUCCESS("Virtual memory manager initialized properly\n");
   
    //kprintf("\n");
/*   
    test_vmm();
    run_kmalloc_tests();
    run_advanced_kmalloc_tests();
    slab_print_all_stats();
*/
    //kprintf("I'm like hey what's up hello\n");
    smp_init();
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
