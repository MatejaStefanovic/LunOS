#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <kernel/spinlock.h>

struct list_node all_tasks; 
struct list_node zombie_tasks;

DEFINE_PER_CPU_GLOBAL(struct task*, current_task);
DEFINE_PER_CPU_GLOBAL(struct list_node, cpu_runqueue);

void scheduler_init(void){
    list_init(&all_tasks);
    list_init(&zombie_tasks);
}

void scheduler_percpu_init(void){
    list_init(&this_core_read(cpu_runqueue));
}

void run_task(struct task* ttr){
    this_core_write(current_task, ttr);
    struct task* curr = this_core_read(current_task);

    load_next_task(&curr->cpu_context);
}

void schedule_task(struct task* task) { 
    if (!task)
        return;
    
    // Add to current CPU's runqueue using tasks_runnable node
    list_add_tail(&task->tasks_runnable, &this_core_read(cpu_runqueue));
}

struct task* get_current_task(void){
    return this_core_read(current_task);
}

extern spinlock task_list_lock;
void schedule(void){
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
        load_next_task(&next->cpu_context);
    }
    load_next_task(&current->cpu_context);
    // If next == current or no runnable tasks, just continue
}
