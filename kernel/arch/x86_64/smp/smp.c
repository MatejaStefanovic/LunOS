#include <kernel/smp.h>
#include <kernel/idt_init.h>
#include <kernel/apic.h>
#include <kernel/memutils.h>
#include <kernel/klogging.h>
#include <kernel/task_manager.h>

uint32_t percpu_processor_ids[MAX_CORES];

void init_percpu_data(uint32_t processor_id) {
    if (processor_id >= MAX_CORES) return;
    
    percpu_processor_ids[processor_id] = processor_id;
    
    // Store pointer to this CPU's processor_id
    uint64_t ptr = (uint64_t)&percpu_processor_ids[processor_id];
    // IMPORTANTE : 0xC0000101 is not an address it is a NAME
    // and it is the name of a register
    asm volatile("wrmsr" 
            : 
            : "c"(0xC0000101), "a"((uint32_t)ptr), "d"((uint32_t)(ptr >> 32)));
}

uint32_t get_current_core_id() {
    uint32_t processor_id;
    asm volatile("movl %%gs:0, %0" : "=r"(processor_id));
    return processor_id;
}

void func(){
    while(1) {
            kprintf("Core: %d, TASK PID: %d\n",get_current_core_id(), this_core_read(current_task)->pid);
            for(volatile int i = 0; i < 100000000; i++); // Simple delay
    }
}
void boot_idle_task(){ 
    struct task *task1 = create_and_schedule_kernel_task(func); 
    struct task *task2 = create_and_schedule_kernel_task(func); 
    apic_timer_enable();

    while(1) {
            kprintf("Core: %d, TASK PID: %d\n",get_current_core_id(), this_core_read(current_task)->pid);
            for(volatile int i = 0; i < 100000000; i++); // Simple delay
    }
}

void ap_entry_point(struct limine_smp_info *cpu_info) {
    reload_idt();
    
    init_percpu_data(cpu_info->processor_id); 
    scheduler_percpu_init();
    
    if (apic_timer_init_cpu(cpu_info->lapic_id) != 0) {
        KERROR("Failed to initialize APIC timer on CPU %u\n", cpu_info->lapic_id);
        hcf();
    }
    
    apic_timer_set_frequency(100);
    struct task *idle_task = create_and_schedule_kernel_task(boot_idle_task); 

    run_kernel_task(idle_task);


    while(1)
        __asm__ __volatile__("pause");
}

// SMP initialization
void smp_init() {
    struct limine_smp_request *mp_request = get_smp_request();
    if (mp_request->response == NULL) {
        kprintf("MP not available, running single-core\n");
        return;
    }
    
    struct limine_smp_response *mp_response = mp_request->response;
    KSUCCESS("Found %lu CPUs\n", mp_response->cpu_count);

    apic_timer_init_cpu(mp_response->bsp_lapic_id);  
    apic_timer_set_frequency(100);

    for (uint64_t i = 0; i < mp_response->cpu_count; i++) {
        struct limine_smp_info *cpu = mp_response->cpus[i];
        
        if (cpu->lapic_id == mp_response->bsp_lapic_id) {     
            init_percpu_data(cpu->processor_id);        
            scheduler_percpu_init();
            continue; // Skip BSP
        }
        continue;
        kprintf("Starting CPU %lu (LAPIC ID: %u)\n", i, cpu->lapic_id);
        cpu->goto_address = ap_entry_point;
    }
    
    struct task *idle_task = create_and_schedule_kernel_task(boot_idle_task); 

    run_kernel_task(idle_task);
}
