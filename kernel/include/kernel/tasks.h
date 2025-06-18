#ifndef __KERNEL_TASKS_H
#define __KERNEL_TASKS_H

#include <kernel/memmgr.h>
#include <kernel/regs.h>
#include <ds/lists.h>

#define TASK_RUNNING 0x0
#define TASK_SLEEPING_INTERRUPTIBLE 0x1
#define TASK_SLEEPING_UNINTERRUPTIBLE 0x2
#define TASK_SLEEPING TASK_SLEEPING_INTERRUPTIBLE | TASK_SLEEPING_UNINTERRUPTIBLE
#define TASK_STOPPED 0x4
#define TASK_TRACED 0x5
#define TASK_ZOMBIE 0x6
#define TASK_DEAD 0x7

struct task {
    uint32_t pid;   // Process ID - always unique
    uint32_t tgid;  // Thread Group ID - used to see if thread belongs to process  

    int priority;
    uint8_t task_state;
    struct regs *cpu_context;

    struct mem_descriptor *md;

    struct task *parent;
    struct task *zombie;

    struct list_node children;
    struct list_node siblings;
    struct list_node zombie_list;

    int exit_code;      // 0 for success, other for errors 
    int exit_signal;    // SIGKILL, SIGSEGV etc.
}

#endif
