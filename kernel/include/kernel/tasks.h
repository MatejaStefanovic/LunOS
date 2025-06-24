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
    struct task_context cpu_context;
    
    uint32_t pid;   // Process ID - always unique
    uint32_t tgid;  // Thread Group ID - used to see if thread belongs to process  

    // From 0 to 100 with 0 being the highest priority
    int priority;
    uint8_t state;

    struct mem_descriptor *md;

    struct task *parent;
    struct task *zombie;

    struct list_node children;
    struct list_node siblings;
    struct list_node zombie_list;

    struct list_node tasks;           // Used to put task into global list of tasks 
    struct list_node tasks_runnable; // Used to put task into CPU specific list of runnable tasks
                                    
    int exit_code;      // 0 for success, other for errors 
    int exit_signal;    // SIGKILL, SIGSEGV etc.
};

// Task creation and initialization
struct task* create_task(void);
struct task* create_kernel_task(void (*func)(void));    

// Task scheduling and state management
void set_task_state(struct task *task, uint8_t state);
void wake_up_task(struct task *task);

// Process/thread management
int fork(struct task *parent);
void exit(int code);
int waitpid(uint32_t pid, int *status);

// Hierarchy and cleanup
void add_child(struct task *parent, struct task *child);
void cleanup_zombie(struct task *task);
struct task* find_task_by_pid(uint32_t pid);

// Signals and error handling
void send_signal(struct task *task, int signal);
void handle_signal(struct task *task);

#endif
