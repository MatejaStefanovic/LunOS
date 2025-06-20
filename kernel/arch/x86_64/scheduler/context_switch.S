# context_switch(struct regs *prev, struct regs *next)
.globl context_switch
.type context_switch, @function
context_switch:
    # Save general-purpose registers into prev
    movq %r15, 0(%rdi)
    movq %r14, 8(%rdi)
    movq %r13, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r11, 32(%rdi)
    movq %r10, 40(%rdi)
    movq %r9,  48(%rdi)
    movq %r8,  56(%rdi)
    movq %rbp, 64(%rdi)
    movq %rsi, 80(%rdi)
    movq %rdx, 88(%rdi)
    movq %rcx, 96(%rdi)
    movq %rbx, 104(%rdi)
    movq %rax, 112(%rdi)
    
    # Save current RSP manually 
    movq %rsp, 184(%rdi)
    
    # Save return address into prev->rip 
    leaq 1f(%rip), %rax
    movq %rax, 144(%rdi)
    
    # Save current flags and segments
    pushfq
    popq %rax
    movq %rax, 160(%rdi)      # Save rflags
    
    movq $0x28, 152(%rdi)     # Save current cs (kernel code)
    movq $0x30, 176(%rdi)     # Save current ss (kernel data)
    movq %rsp, 168(%rdi)      # Save current userrsp (same as rsp for kernel)
    
    # NOW save rdi - we're done using it as a pointer
    movq %rdi, 72(%rdi)
    
    # Load next task's registers
    movq %rsi, %rax           # Save next pointer in %rax
    movq 0(%rax), %r15
    movq 8(%rax), %r14
    movq 16(%rax), %r13
    movq 24(%rax), %r12
    movq 32(%rax), %r11
    movq 40(%rax), %r10
    movq 48(%rax), %r9
    movq 56(%rax), %r8
    movq 64(%rax), %rbp
    movq 88(%rax), %rdx
    movq 96(%rax), %rcx
    movq 104(%rax), %rbx
    
    # Set up iretq frame
    movq 184(%rax), %rsp      # Load next task's RSP
    
    # Build iretq frame (SS, RSP, RFLAGS, CS, RIP)
    pushq 176(%rax)           # ss
    pushq 168(%rax)           # userrsp
    pushq 160(%rax)           # rflags
    pushq 152(%rax)           # cs
    pushq 144(%rax)           # rip (return address we saved as 1f)
    
    # Load remaining registers
    movq 72(%rax), %rdi
    movq 80(%rax), %rsi
    movq 112(%rax), %rax      # Load %rax last
    
    # Atomically switch context and restore flags
    iretq
1:
    ret


# initial_context_load(struct regs *regs) - for first-time task loading
.globl initial_context_load  
.type initial_context_load, @function
initial_context_load:
    # Load manually pushed registers
    movq 0(%rdi), %r15
    movq 8(%rdi), %r14
    movq 16(%rdi), %r13
    movq 24(%rdi), %r12
    movq 32(%rdi), %r11
    movq 40(%rdi), %r10
    movq 48(%rdi), %r9
    movq 56(%rdi), %r8
    movq 64(%rdi), %rbp
    movq 88(%rdi), %rdx
    movq 96(%rdi), %rcx
    movq 104(%rdi), %rbx
    movq 112(%rdi), %rax
    
    # Set up the stack for iretq
    # Load the target RSP first
    movq 184(%rdi), %rsp
    
    # Push the interrupt frame in reverse order (SS, RSP, RFLAGS, CS, RIP)
    pushq 176(%rdi)    # ss
    pushq 168(%rdi)    # userrsp  
    pushq 160(%rdi)    # rflags (with IF bit set)
    pushq 152(%rdi)    # cs
    pushq 144(%rdi)    # rip
    
    # Load the remaining registers
    movq 80(%rdi), %rsi
    movq 72(%rdi), %rdi   # Load rdi last
    
    # Atomically restore context and jump
    iretq
