#ifndef __KERNEL_SCHEDULER_H
#define __KERNEL_SCHEDULER_H

#include <kernel/tasks.h>
#include <kernel/regs.h>
#include <kernel/smp.h>

extern struct list_node all_tasks;

DECLARE_PER_CPU(struct list_node, cpu_runqueue);
DECLARE_PER_CPU(struct task*, current_task);

void scheduler_init(void);
void sched_task(struct task* t, int cpu_id);
void sched_remove_task(struct task* t);

void schedule(void);

struct task* get_current_task(void);

extern void load_next_task(struct task_context* cont);
void debug_print_all_runqueues(void);

#endif
