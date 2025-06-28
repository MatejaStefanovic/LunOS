#include <kernel/task_manager.h>

DEFINE_PER_CPU(int, task_counter);
DEFINE_SPINLOCK(task_counter_lock);
extern int total_cpus;

void init_task_ctr(int cpu_count){
    for(int i = 0; i < cpu_count; i++)
        __percpu_task_counter[i] = 0;
}

static int find_least_busy_cpu(void){
    int id = 0;

    spinlock_lock(&task_counter_lock); 
    int min = __percpu_task_counter[0];
    for(int i = 1; i < 4; i++){
        if(__percpu_task_counter[i] < min){
            min = __percpu_task_counter[i];
            id = i;
        }
    }
    __percpu_task_counter[id] += 1;
    spinlock_unlock(&task_counter_lock);

    return id;
}

struct task* create_and_schedule_kernel_task(void (*func)(void)){
    struct task* t = create_kernel_task(func);
    int id = find_least_busy_cpu();
    sched_task(t, id);
    return t;
}

void run_kernel_task(struct task* t){ 
    run_task(t); 
}

void task_exit(int exit_code) {
    struct task *current = get_current_task();

    int_flags flags = save_and_disable_interrupts();

    current->exit_code = exit_code;
    current->state = TASK_ZOMBIE;

    sched_remove_task(current);

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
    task_orphan_children(current);

    // Go to next task
    schedule();
}
