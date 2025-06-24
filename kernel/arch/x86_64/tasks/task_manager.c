#include <kernel/task_manager.h>

struct task* create_and_schedule_kernel_task(void (*func)(void)){
    struct task* t = create_kernel_task(func);
    schedule_task(t);
    return t;
}

void run_kernel_task(struct task* t){ 
    run_task(t); 
}
