#include <kernel/tasks.h>
#include <kernel/spinlock.h>
#include <kernel/pmm.h>
#include <kernel/smp.h>
#include <string.h>

DEFINE_SPINLOCK(task_list_lock);
extern struct list_node all_tasks; 
extern struct list_node zombie_tasks;

#define PID_MAX 1111111111  // Wrap around if we reach **I sincerely hope this never happens**
uint32_t pid_counter = 1;

DEFINE_SPINLOCK(pid_ctr_lock);
uint32_t incr_pid_ctr(){
    spinlock_lock(&pid_ctr_lock);

    uint32_t pid = pid_counter++;
    
    if (pid_counter > PID_MAX) 
        pid_counter = 1;  // 0 is for INIT
    
    spinlock_unlock(&pid_ctr_lock);
    
    return pid;
}

struct task* create_task(){
    struct task *task = kmalloc(sizeof(struct task));
    if(!task)
        return NULL;

    task->pid =  incr_pid_ctr();
    task->tgid = task->pid;
    
    // Background task by default (for kernel use)
    task->priority = 99;
    // Backround task is immediately runnable
    task->state = TASK_RUNNING;
    // Kernel tasks share the kernel address space and don't require
    // a memory descriptor (while I could do it as kernel address space does exist)
    // I find it unnecessary to allocate some space for no reason
    // MD allocation will be handled separately depending on which function
    // uses create_task (fork, execv, clone etc.) 
    task->md = NULL;  
    
    memset(&task->cpu_context, 0, sizeof(task->cpu_context));

    task->zombie = NULL;
    task->parent = NULL; // Will later be INIT when I end up making it

    list_init(&task->siblings); // When INIT is added this will just be the list of its children
    list_init(&task->children);
    list_init(&task->zombie_list);


    task->exit_code = 0;
    task->exit_signal = 0;

    return task;
}

#define KERNEL_STACK_SIZE 4*PAGE_SIZE
struct task* create_kernel_task(void (*func)(void)) {
    
    struct task *ktask = create_task();
    
    if(!ktask)
        return NULL;

    // We need to allocate a kernel stack for this to work
    void* stack = kmalloc(KERNEL_STACK_SIZE);
    if (!stack){
        kfree(ktask); // Clean up what we already allocated
        return NULL;
    }

    // Set up stack pointer at top of allocated stack
    ktask->cpu_context.stack_ptr = ((uint64_t)stack + KERNEL_STACK_SIZE);
    ktask->cpu_context.rip = (uint64_t)func;
    ktask->cpu_context.rflags = 0x202;     // IF=1 + reserved bit
    ktask->cpu_context.cs = 0x28;          // Kernel code segment
    ktask->cpu_context.ss = 0x30;          // Kernel data segment       

    list_init(&ktask->tasks_runnable);
    
    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_add_tail(&ktask->tasks, &all_tasks); 
    spinlock_unlock_intrestore(&task_list_lock, flags);
    
    return ktask;
}

void set_task_state(struct task *task, uint8_t state){
    task->state = state;
}

void wake_up_task(struct task *task){
    task->state = TASK_RUNNING;
}

void send_signal(struct task *task, int signal){
    task->exit_signal = signal;
} 
