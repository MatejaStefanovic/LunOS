.globl load_next_task
.type load_next_task, @function
load_next_task:
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
    movq 80(%rdi), %rsi   
    movq 88(%rdi), %rdx  
    movq 96(%rdi), %rcx  
    movq 104(%rdi), %rbx 
    movq 112(%rdi), %rax

    pushq 152(%rdi) # SS - if needed
    pushq 144(%rdi) # RSP - if needed
    pushq 136(%rdi) # RFLAGS    
    pushq 128(%rdi) # CS  
    pushq 120(%rdi) # RIP     
    
    movq 72(%rdi), %rdi    
    
    iretq

.globl build_iretq_frame 
.type build_iretq_frame, @function
build_iretq_frame:
    pushq 152(%rdi) # SS - if needed
    pushq 144(%rdi) # RSP - if needed
    pushq 136(%rdi) # RFLAGS    
    pushq 128(%rdi) # CS  
    pushq 120(%rdi) # RIP     
    
    iretq

