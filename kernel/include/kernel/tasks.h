#ifndef __KERNEL_TASKS_H
#define __KERNEL_TASKS_H

#include <kernel/memmgr.h>
#include <kernel/regs.h>
#include <kernel/spinlock.h>
#include <ds/lists.h>

#define TASK_RUNNING 0x0
#define TASK_SLEEPING_INTERRUPTIBLE 0x1
#define TASK_SLEEPING_UNINTERRUPTIBLE 0x2
#define TASK_SLEEPING TASK_SLEEPING_INTERRUPTIBLE | TASK_SLEEPING_UNINTERRUPTIBLE
#define TASK_STOPPED 0x4
#define TASK_TRACED 0x5
#define TASK_ZOMBIE 0x6
#define TASK_DEAD 0x7

extern spinlock task_list_lock;

struct task {
    // MUST BE FIRST!!! The way we save 
    // currently running task is by getting 
    // a pointer to our __percpu_current_task[MAXCORES] 
    // array in our ISR_STUBS and we need
    // the offset of cpu context so the easiet
    // way to do that is to always make it 0
    struct task_context cpu_context;
    
    uint32_t pid;   // Process ID - always unique
    uint32_t tgid;  // Thread Group ID - used to see if thread belongs to process  

    int cpu_id;
    // From 0 to 100 with 0 being the highest priority
    int priority;
    uint8_t state;

    struct mem_descriptor *md;

    void* kernel_stack_base;
    struct task *parent;

    struct list_node children;        // For live children
    struct list_node siblings;        // To link into parent's children
    struct list_node zombie_children; // For dead children (as parent)
    struct list_node zombie_siblings;  // To link into parent's zombie_children (as child)
    

    struct list_node tasks;           // Used to put task into global list of tasks 
    struct list_node tasks_runnable; // Used to put task into CPU specific list of runnable tasks
                                    
    int exit_code;      // 0 for success, other for errors 
    int exit_signal;    // SIGKILL, SIGSEGV etc.
};

// Task creation and initialization
struct task* create_task(void);
struct task* create_kernel_task(void (*func)(void));    
struct task* create_user_task(void);

void task_orphan_children(struct task* parent); 
void task_destroy(struct task* task);

// Process/thread management
int fork(struct task *parent);
int waitpid(uint32_t pid, int *status);

// Hierarchy and cleanup
void task_add_child(struct task* parent, struct task* child);
void task_add_zombie(struct task* parent, struct task* zombie);
void task_remove_child(struct task* parent, struct task* child);
void task_remove_zombie(struct task* parent, struct task* zombie);


#endif
