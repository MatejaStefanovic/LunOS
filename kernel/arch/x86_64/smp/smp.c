#include <kernel/smp.h>
#include <kernel/idt_init.h>
#include <kernel/apic.h>
#include <kernel/memutils.h>
#include <kernel/klogging.h>

void ap_entry_point(struct limine_smp_info *cpu_info) {
    reload_idt();
    
    // Enhanced APIC setup with verification - pass the LAPIC ID directly
    if (apic_timer_init_cpu(cpu_info->lapic_id) != 0) {
        KERROR("Failed to initialize APIC timer on CPU %u\n", cpu_info->lapic_id);
        hcf();
    }
    
    asm volatile ("sti");
    KSUCCESS("CPU %u online with verified APIC timer\n", cpu_info->lapic_id);
    
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
