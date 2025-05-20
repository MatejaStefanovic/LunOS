#ifndef __KERNEL_ISR_DISPATCHER_H
#define __KERNEL_ISR_DISPATCHER_H

#include <kernel/exception_handlers.h>
#include <kernel/irq_handlers.h>

void isr_dispatch(struct regs_t *);

#endif
