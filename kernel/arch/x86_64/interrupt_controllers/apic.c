#include <kernel/apic.h>
#include <kernel/timer.h>
#include <kernel/vmm.h> 
#include <kernel/klogging.h>
#include <kernel/idt_init.h>
#include <kernel/smp.h>
#include <kernel/scheduler.h>

#include <string.h>

volatile uint32_t *apic_base = NULL;
static uint32_t apic_timer_frequency = 0;
DEFINE_PER_CPU_VOLATILE(uint64_t, timer_ticks);

// For BSP use
int apic_global_init() {
    // Map APIC base - this only needs to be done once
    virt_addr apic_vaddr = 0xFFFFFF8000000000UL;
    uint64_t mmio_flags = PTE_PRESENT | PTE_WRITABLE | PTE_CACHE_DISABLE | PTE_WRITETHROUGH; 
    
    struct addr_space *kernel_as = get_kernel_as();
    
    int result = vmm_map_range(kernel_as, apic_vaddr, APIC_BASE_ADDR, 0x1000, mmio_flags);
    if (result != 0) {
        KERROR("Failed to map APIC base address\n");
        return -1;
    }
    
    apic_base = (volatile uint32_t*)apic_vaddr;
    KSUCCESS("APIC mapped at virtual address 0x%lx\n", apic_vaddr);
    
    // Calibrate timer using PIT (only needs to be done once)
    apic_timer_calibrate();
    
    return 0;
}

// For CPU use
int apic_timer_init_cpu(uint32_t cpu_id) {
    if (!apic_base) {
        KERROR("APIC not globally initialized\n");
        return -1;
    }
     
    // APIC might already be enabled so we check that 
    uint32_t spurious = apic_read(APIC_SPURIOUS_VECTOR);
    
    spurious |= 0x100; // Enable APIC
    spurious &= ~0xFF;  // Clear spurious vector bits
    spurious |= 0xFF;   // Set spurious vector to 255
    apic_write(APIC_SPURIOUS_VECTOR, spurious);
    
    // Verify APIC is now enabled
    uint32_t spurious_check = apic_read(APIC_SPURIOUS_VECTOR);
    if (!(spurious_check & 0x100)) {
        KERROR("CPU %u: Failed to enable APIC (spurious=0x%x)\n", cpu_id, spurious_check);
        return -1;
    }
    
    // Clear any pending interrupts in Local Vector Table
    apic_write(APIC_TIMER_LVT, 0x10000);      // Mask timer
    apic_write(APIC_LINT0, 0x10000);          // Mask LINT0
    apic_write(APIC_LINT1, 0x10000);          // Mask LINT1
    apic_write(APIC_ERROR, 0x10000);          // Mask error
    
    // Clear error status
    apic_write(APIC_ERROR_STATUS, 0);
    apic_write(APIC_ERROR_STATUS, 0); // Intel manual says write twice XDDDDDDDDDDDD
    
    // Check for any error conditions
    uint32_t error_status = apic_read(APIC_ERROR_STATUS);
    if (error_status != 0) {
        KWARN("CPU %u: APIC error status: 0x%x\n", cpu_id, error_status);
    }
    
    // Send EOI to clear any pending interrupts
    apic_write(APIC_EOI, 0);
    
    
    return 0;
}

extern void isr64(void);
void apic_timer_register_handler() {
    create_gate_entry(APIC_TIMER_VECTOR, isr64, 0x28, 0x8E);
}

// We use PIT to calibrate for better precision
void apic_timer_calibrate() {
    KSUCCESS("Calibrating APIC timer using PIT...\n");
    
    // Disable APIC timer during calibration
    apic_write(APIC_TIMER_LVT, 0x10000);
    
    // Set APIC timer divide by 16
    apic_write(APIC_TIMER_DIVIDE, 0x3);
    
    // Set APIC timer to maximum count
    apic_write(APIC_TIMER_INITIAL, 0xFFFFFFFF);
    
    cpu_wait_10ms(); 
    uint32_t apic_current = apic_read(APIC_TIMER_CURRENT);
    uint32_t apic_ticks_in_10ms = 0xFFFFFFFF - apic_current;
    
    // Calculate APIC timer frequency
    // apic_ticks_in_10ms ticks in 10ms = frequency * 0.01
    apic_timer_frequency = apic_ticks_in_10ms * 100;
    
    // Stop the timer
    apic_write(APIC_TIMER_LVT, 0x10000);
    apic_write(APIC_TIMER_INITIAL, 0);
    
    KSUCCESS("APIC timer calibrated: %u ticks per second\n", apic_timer_frequency);
}


void apic_timer_set_frequency(uint32_t frequency) {
    if (apic_timer_frequency == 0) {
        KERROR("APIC timer not calibrated\n");
        return;
    }
    
    if (frequency == 0) {
        KERROR("Invalid frequency: 0\n");
        return;
    }
    
    uint32_t initial_count = apic_timer_frequency / frequency;
    
    if (initial_count == 0) {
        KWARN("Frequency too high, setting to minimum\n");
        initial_count = 1;
    }
    
    
    apic_write(APIC_TIMER_DIVIDE, 0x3);
    
    apic_write(APIC_TIMER_LVT, APIC_TIMER_PERIODIC | APIC_TIMER_VECTOR | APIC_TIMER_LVT_MASKED);
    
    apic_write(APIC_TIMER_INITIAL, initial_count);
    
}
void apic_timer_handler() {
    apic_write(APIC_EOI, 0);
   
    schedule();
}

void apic_timer_enable() {
    uint32_t lvt = apic_read(APIC_TIMER_LVT);
    lvt &= ~0x10000; // Clear mask bit
    apic_write(APIC_TIMER_LVT, lvt);
}

void apic_timer_disable() {
    uint32_t lvt = apic_read(APIC_TIMER_LVT);
    lvt |= 0x10000; // Set mask bit
    apic_write(APIC_TIMER_LVT, lvt);
}

// Get current tick count
uint64_t apic_timer_get_ticks() {
    return this_core_read(timer_ticks);
}
