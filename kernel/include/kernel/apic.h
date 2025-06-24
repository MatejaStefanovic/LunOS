#ifndef __KERNEL_APIC_H
#define __KERNEL_APIC_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/regs.h>

#define APIC_BASE_ADDR          0xFEE00000
// APIC register offsets from our base ^
#define APIC_ID                 0x20
#define APIC_VERSION            0x30
#define APIC_TASK_PRIORITY      0x80
#define APIC_PROCESSOR_PRIORITY 0xA0
#define APIC_EOI                0xB0
#define APIC_LOGICAL_DEST       0xD0
#define APIC_DEST_FORMAT        0xE0
#define APIC_SPURIOUS_VECTOR    0xF0
#define APIC_ERROR_STATUS       0x280
#define APIC_LINT0              0x350
#define APIC_LINT1              0x360
#define APIC_ERROR              0x370
#define APIC_TIMER_LVT          0x320
#define APIC_TIMER_INITIAL      0x380
#define APIC_TIMER_CURRENT      0x390
#define APIC_TIMER_DIVIDE       0x3E0

// APIC Timer Modes
#define APIC_TIMER_ONESHOT      0x00000000
#define APIC_TIMER_LVT_MASKED   0x10000
#define APIC_TIMER_PERIODIC     0x00020000
#define APIC_TIMER_TSC_DEADLINE 0x00040000
#define APIC_TIMER_VECTOR       0x40

int apic_global_init(void);
int apic_timer_init_cpu(uint32_t cpu_id);
void apic_timer_register_handler(void);

void apic_timer_calibrate(void);
void apic_timer_set_frequency(uint32_t frequency);

void apic_timer_enable(void);
void apic_timer_disable(void);
void apic_timer_handler(void);
uint64_t apic_timer_get_ticks(void);

// A little bit of fancy macro stuff
#define apic_read(reg) (apic_base ? apic_base[(reg) >> 2] : 0)
#define apic_write(reg, value) do { \
    if (apic_base) { \
        apic_base[(reg) >> 2] = (value); \
    } \
} while(0)

// Debug
int apic_timer_test(uint32_t cpu_id);

#endif 
