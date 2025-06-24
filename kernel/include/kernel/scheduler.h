#ifndef __KERNEL_SCHEDULER_H
#define __KERNEL_SCHEDULER_H

#include <kernel/tasks.h>
#include <kernel/regs.h>
#include <kernel/smp.h>

extern struct list_node all_tasks;
extern struct list_node zombie_tasks; 

DECLARE_PER_CPU(struct list_node, cpu_runqueue);
DECLARE_PER_CPU(struct task*, current_task);

void scheduler_init(void);
void scheduler_percpu_init(void);
void schedule_first_task(struct task* idle_task);
void schedule_next_task(struct task* next_task);
void schedule(void);


extern void load_next_task(struct task_context* cont);

#endif
