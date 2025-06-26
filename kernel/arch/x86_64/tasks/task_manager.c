#include <kernel/task_manager.h>

struct task* create_and_schedule_kernel_task(void (*func)(void)){
    struct task* t = create_kernel_task(func);
    sched_task(t);
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
