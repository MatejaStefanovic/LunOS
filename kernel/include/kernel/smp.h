#ifndef __KERNEL_SMP_H
#define __KERNEL_SMP_H

#include <kernel/limine_requests.h>

extern int total_cpus;

#define MAX_CORES 4 

#define DECLARE_PER_CPU(type, name) \
    extern type __percpu_##name[MAX_CORES]

#define DEFINE_PER_CPU(type, name) \
    static type __percpu_##name[MAX_CORES]

#define DEFINE_PER_CPU_GLOBAL(type, name) \
    type __percpu_##name[MAX_CORES]

#define DEFINE_PER_CPU_VOLATILE(type, name) \
    static volatile type __percpu_##name[MAX_CORES]

#define this_core_read(var) \
    (__percpu_##var[get_current_core_id()])

#define this_core_write(var, value) \
    (__percpu_##var[get_current_core_id()] = (value))

void smp_init(void);
uint32_t get_current_core_id(void);

#endif
