#include <kernel/task_manager.h>

DEFINE_PER_CPU(int, task_counter);
static DEFINE_SPINLOCK(task_counter_lock);
extern int total_cpus;

void init_task_ctr(int cpu_count){
    for(int i = 0; i < cpu_count; i++)
        __percpu_task_counter[i] = 0;
}

static int find_least_busy_cpu(void){
    int id = 0;
    int_flags flags;
    spinlock_lock_intsave(&task_counter_lock, &flags); 
    int min = __percpu_task_counter[0];
    for(int i = 1; i < MAX_CORES; i++){
        if(__percpu_task_counter[i] < min){
            min = __percpu_task_counter[i];
            id = i;
        }
    }
    __percpu_task_counter[id] += 1;
    spinlock_unlock_intrestore(&task_counter_lock, flags);

    return id;
}

struct task* create_and_schedule_kernel_task(void (*func)(void)){
    struct task* t = create_kernel_task(func);
    int id = find_least_busy_cpu();
    t->cpu_id = id;
    sched_task(t, id);
    return t;
}

void task_exit(int exit_code) {
    struct task *current = get_current_task();

    save_and_disable_interrupts();

    current->exit_code = exit_code;
    current->state = TASK_ZOMBIE;

    sched_remove_task(current);
    __percpu_task_counter[current->cpu_id] -= 1;

    // Free if non kernel space task
    if (current->md) {
        mm_free(current->md);
        current->md = NULL;
    }

    // Wake parent if it's waiting
    if (current->parent && current->parent->state == TASK_SLEEPING_INTERRUPTIBLE) {    
        task_add_zombie(current->parent, current);
        // TODO: this below - tricky part is what if parent is on another CPU 
        //wake_up_task(current->parent);
    }

    // If it has children orphan them
    if(&current->children != current->children.next)
        task_orphan_children(current);

    // Go to next task
    schedule();
}

void wake_up_task(struct task* task){
    task->state = TASK_RUNNING;
    // run it on the CPU that is least busy 
    int id = find_least_busy_cpu();
    task->cpu_id = id;
    sched_task(task, id); 
}
