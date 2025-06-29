#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <kernel/spinlock.h>

struct list_node all_tasks; 

DEFINE_PER_CPU_GLOBAL(struct task*, current_task);
DEFINE_PER_CPU_GLOBAL(struct list_node, cpu_runqueue);
DEFINE_PER_CPU(spinlock, runqueue_lock);

void scheduler_init(void){
    list_init(&all_tasks);
    
    for(int i = 0; i < MAX_CORES; i++){
        list_init(&__percpu_cpu_runqueue[i]);
        __percpu_runqueue_lock[i] = (spinlock)SPINLOCK_INIT;
        __percpu_current_task[i] = NULL;
    }
}

void sched_task(struct task* task, int cpu_id) { 
    if (!task)
        return;
    
    // Add to current CPU's runqueue using tasks_runnable node
    // note: it isn't necessary to disable interrupts but it will 
    // prove useful later when I switch to a priority scheduler or 
    // if we switch to a task that calls sched_task immediately
    // (makes it not waste a lot of cycles before the task that locked
    // releases the lock
    int_flags flags;
    spinlock_lock_intsave(&__percpu_runqueue_lock[cpu_id], &flags);
    list_add_tail(&task->tasks_runnable, &__percpu_cpu_runqueue[cpu_id]);
    spinlock_unlock_intrestore(&__percpu_runqueue_lock[cpu_id], flags);
}

void sched_remove_task(struct task* task){
    if(!task)
        return;

    // Same story as task_sched
    int_flags flags;
    spinlock_lock_intsave(&__percpu_runqueue_lock[task->cpu_id], &flags);
    list_del(&task->tasks_runnable);
    spinlock_unlock_intrestore(&__percpu_runqueue_lock[task->cpu_id], flags);
}

struct task* get_current_task(void){
    return this_core_read(current_task);
}

extern spinlock task_list_lock;
void schedule(void){
    struct task *current = this_core_read(current_task);
    struct task *next = NULL;

    int_flags flags;
    spinlock_lock_intsave(&this_core_read(runqueue_lock), &flags);

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

    spinlock_unlock_intrestore(&this_core_read(runqueue_lock), flags);
    
    if (next && next != current) {
        this_core_write(current_task, next);
        load_next_task(&next->cpu_context);
    }
    load_next_task(&current->cpu_context);
    // If next == current or no runnable tasks, just continue
}










static void debug_print_runqueue(int cpu_id) {
    kprintf("=== CPU %d Runqueue ===\n", cpu_id);
    
    spinlock_lock(&__percpu_runqueue_lock[cpu_id]);
    
    struct list_node *runqueue = &__percpu_cpu_runqueue[cpu_id];
    
    if (list_empty(runqueue)) {
        kprintf("  Runqueue is EMPTY\n");
        spinlock_unlock(&__percpu_runqueue_lock[cpu_id]);
        return;
    }
    
    int count = 0;
    struct list_node *current = runqueue->next;
    
    kprintf("  Runqueue head: %p (next: %p, prev: %p)\n", 
            runqueue, runqueue->next, runqueue->prev);
    
    while (current != runqueue) {
        struct task *task = container_of(current, struct task, tasks_runnable);
        kprintf("  [%d] Task PID: %d, addr: %p, node: %p (next: %p, prev: %p)\n", 
                count, task->pid, task, current, current->next, current->prev);
        
        current = current->next;
        count++;
        
        // Safety check to prevent infinite loops
        if (count > 100) {
            kprintf("  ERROR: Runqueue traversal exceeded 100 nodes - possible corruption!\n");
            break;
        }
    }
    
    kprintf("  Total tasks in runqueue: %d\n", count);
    spinlock_unlock(&__percpu_runqueue_lock[cpu_id]);
}

// Helper to print all CPU runqueues
void debug_print_all_runqueues(void) {
    for (int i = 0; i < 4; i++) {
        debug_print_runqueue(i);
    }
}
