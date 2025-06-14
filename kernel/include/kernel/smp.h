#ifndef __KERNEL_SMP_H
#define __KERNEL_SMP_H

#include <kernel/limine_requests.h>

void smp_init(void);
void ap_entry_point(struct limine_smp_info *cpu_info);

#endif
