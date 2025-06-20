#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <kernel/spinlock.h>

struct list_node all_tasks; 
struct list_node zombie_tasks;

DEFINE_PER_CPU(struct task*, current_task);
DEFINE_PER_CPU_GLOBAL(struct list_node, cpu_runqueue);

void func(){
    while(1) {
            kprintf("Core: %d, Task: %d ",get_current_core_id(), this_core_read(current_task)->pid);
            for(volatile int i = 0; i < 100000000; i++); // Simple delay
            kprintf("\n");
    }
}

void scheduler_init(){
    list_init(&all_tasks);
    list_init(&zombie_tasks);
}

void scheduler_percpu_init(){
    list_init(&this_core_read(cpu_runqueue));
    this_core_read(current_task) = NULL;
}

struct task *get_current_task(){
    return this_core_read(current_task);
}

extern spinlock task_list_lock;
// TODO: change to priority scheduler at some point
void schedule(){
    struct task *current = this_core_read(current_task);
    struct task *next = NULL;
    struct list_node *runqueue = &this_core_read(cpu_runqueue);
    
    // If no current task, pick the first runnable task from this CPU's queue
    if (!current) {
        if (!list_empty(runqueue)) {
            struct list_node *first = runqueue->next;
            next = container_of(first, struct task, tasks_runnable);
        }
    } else {
        // Find next task
        struct list_node *current_node = &current->tasks_runnable;
        struct list_node *next_node = current_node->next;
        
        // wrap to beginning 
        if (next_node == runqueue) {
            next_node = runqueue->next;
        }
        
        // If this fails we have an empty run queue
        if (next_node != runqueue) {
            next = container_of(next_node, struct task, tasks_runnable);
        }
    }
    
    if (next && next != current) {
        this_core_write(current_task, next);
        
        if (current) {
            // bane of my existence in the form of a function
            context_switch(current->cpu_context, next->cpu_context);
        } else {
            // If this is the first task we're scheduling load initial task context
            initial_context_load(next->cpu_context);
        }
    }
    // If next == current or no runnable tasks, just continue
}
