#ifndef __KERNEL_SCHEDULER_H
#define __KERNEL_SCHEDULER_H

#include <kernel/tasks.h>
#include <kernel/regs.h>
#include <kernel/smp.h>

extern struct list_node all_tasks;
extern struct list_node zombie_tasks; 

DECLARE_PER_CPU(struct list_node, cpu_runqueue);

struct task *get_current_task();
void scheduler_init(void);
void scheduler_percpu_init(void);
void schedule(void);

extern void context_switch(struct regs *prev, struct regs *next); 
extern void initial_context_load(struct regs *r);
extern void func();

#endif
