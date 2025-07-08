#include <kernel/tasks.h>
#include <kernel/pmm.h>
#include <kernel/smp.h>
#include <klib/string.h>


DEFINE_SPINLOCK(task_list_lock);
static DEFINE_SPINLOCK(pid_ctr_lock);

extern struct list_node all_tasks; 
extern struct list_node zombie_tasks;

#define PID_MAX 1111111111  // Wrap around if we reach **I sincerely hope this never happens**
static uint32_t pid_counter = 1;   // Start at 1 for INIT

static uint32_t incr_pid_ctr(void){
    spinlock_lock(&pid_ctr_lock);

    uint32_t pid = pid_counter++;
    
    if (pid_counter > PID_MAX) 
        pid_counter = 2;  // 1 is for INIT
    
    spinlock_unlock(&pid_ctr_lock);
    
    return pid;
}

struct task* create_task(void){
    struct task *task = kmalloc(sizeof(*task));
    if(!task)
        return NULL;

    task->pid = 0;
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

    task->parent = NULL; // Will later be INIT when I end up making it

    list_init(&task->siblings);
    list_init(&task->children);
    list_init(&task->zombie_siblings);
    list_init(&task->zombie_children);

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
        kfree(ktask); // Free what we already allocated
        return NULL;
    }

    // Set up stack pointer at top of allocated stack
    ktask->kernel_stack_base = stack; // Keep track of base so we know where to free later
    ktask->cpu_context.stack_ptr = ((uint64_t)stack + KERNEL_STACK_SIZE);
    ktask->cpu_context.rip = (uint64_t)func;
    ktask->cpu_context.rflags = 0x202;     // IF=1 + reserved bit
    ktask->cpu_context.cs = 0x08;          // Kernel code segment
    ktask->cpu_context.ss = 0x10;          // Kernel data segment       

    list_init(&ktask->tasks_runnable);
    
    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_add_tail(&ktask->tasks, &all_tasks); 
    spinlock_unlock_intrestore(&task_list_lock, flags);
    
    return ktask;
}

// Will later take some info from the ELF loader
// such as user task entry point, size of code and 
// data segment etc. this is just a prototype 
struct task* create_user_task(void){
    struct task *utask = create_task();
    
    if(!utask)
        return NULL;

    utask->pid = incr_pid_ctr();
    utask->tgid = utask->pid;

    utask->md = kmalloc(sizeof(*utask->md));
    if(!utask->md){
        kfree(utask); // Free what we already allocated
        return NULL;
    }
    /*
     * Here should go stuff like 
     * mm_set_executable() but I'll only add this stuff
     * once I set up VFS and ELF loader, for now kernel 
     * task scheduling seems to work despite the weird
     * interrupt CPU behavior
    */

    list_init(&utask->tasks_runnable);
    
    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_add_tail(&utask->tasks, &all_tasks); 
    spinlock_unlock_intrestore(&task_list_lock, flags);
    
    return utask;
}

void task_destroy(struct task* task) {
    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_del(&task->tasks);        // Remove from global list
    list_del(&task->siblings);     // Remove from parent's children
    spinlock_unlock_intrestore(&task_list_lock, flags);
    
    // Free if non kernel space task
    if (task->md) 
        mm_free(task->md);
    
    if (task->kernel_stack_base)
        kfree(task->kernel_stack_base); 
    
    kfree(task);
}

void task_add_child(struct task* parent, struct task* child){
    if(child->parent != parent){
        KERROR("Child's parent isn't the same as provided parent, something might be NULL\n");
        return;
    }

    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_add_tail(&child->siblings, &parent->children);
    spinlock_unlock_intrestore(&task_list_lock, flags);
}

void task_remove_child(struct task* parent, struct task* child){
    if(child->parent != parent){
        KERROR("Child's parent isn't the same as provided parent, something might be NULL\n");
        return;
    }

    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_del(&child->siblings);
    spinlock_unlock_intrestore(&task_list_lock, flags);
}

void task_add_zombie(struct task* parent, struct task* zombie){
    if(zombie->parent != parent){
        KERROR("Zombie's parent isn't the same as provided parent, something might be NULL\n");
        return;
    }

    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_del(&zombie->siblings); // Remove from parent's children
    list_add_tail(&zombie->zombie_siblings, &parent->zombie_children); // Add to zombie children
    spinlock_unlock_intrestore(&task_list_lock, flags);
}

void task_remove_zombie(struct task* parent, struct task* zombie){
    if(zombie->parent != parent){
        KERROR("Zombie's parent isn't the same as provided parent, something might be NULL\n");
        return;
    }

    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    list_del(&zombie->zombie_siblings);
    spinlock_unlock_intrestore(&task_list_lock, flags);
}

void task_orphan_children(struct task* dead_parent){
    int_flags flags;
    spinlock_lock_intsave(&task_list_lock, &flags);
    for(struct list_node* node = dead_parent->children.next; node != &dead_parent->children;){
        struct list_node* node_to_remove = node; 
        node = node->next;
        //struct task* child = container_of(node_to_remove, struct task, siblings);
        //child->parent = init; // Don't have init yet
        list_del(node_to_remove); 
        //list_add_tail(child->siblings, init->children); // Don't have init
    }

    spinlock_unlock_intrestore(&task_list_lock, flags);
}
