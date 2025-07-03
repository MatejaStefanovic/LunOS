.section .text
.extern isr_common_stub

.macro ISR_NOERR n
.global isr\n
isr\n:
    cli                 # Disable interrupts
    pushq $0            # Fake error code (0) 
    pushq $\n           # Interrupt number (n) 
    jmp isr_common_stub # Jump to common handler
.endm

.macro ISR_ERR n
.global isr\n
isr\n:
    cli                 # Disable interrupts
    pushq $\n           # Interrupt number (n) 
    jmp isr_common_stub # Jump to common handler
.endm

# Define stubs for exceptions (0-31)
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14     # Page Fault (has real error code)
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

# We're in the IRQ territory now
ISR_NOERR 64

.extern __percpu_current_task

.global isr_common_stub
isr_common_stub:
    # Save rbx and rax temporarily
    pushq %rax

    # Get current CPU ID from GS segment
    movl %gs:[0], %eax         # Get CPU ID (32-bit)
    # Note: we automatically sign extend to rax

    # __percpu_current_task[MAXCORES] is an array of
    # pointers to task struct, therefore we can simply
    # move across that array with rax * sizeof(task*) which is 8
    # by design task->cpu_context is at offset 0 which means that 
    # after getting the correct task from the percpu array we immediately
    # get a pointer to cpu_context
    
    movq __percpu_current_task(, %rax, 8), %rax # ptr to cpu_context of a task
    
    movq %r15, 0(%rax)
    movq %r14, 8(%rax)
    movq %r13, 16(%rax)
    movq %r12, 24(%rax)
    movq %r11, 32(%rax)
    movq %r10, 40(%rax)
    movq %r9,  48(%rax)
    movq %r8,  56(%rax)

    movq %rbp, 64(%rax)
    movq %rdi, 72(%rax)
    movq %rsi, 80(%rax)
    movq %rdx, 88(%rax)
    movq %rcx, 96(%rax)
    movq %rbx, 104(%rax)
        
    # We already saved rcx into our tasks context so it's fine to do this:
    popq %rcx # This now has our rax value put into it
    
    movq %rcx, 112(%rax)    # Save our original rax

    movq 16(%rsp), %rcx     # Get RIP from stack
    movq %rcx, 120(%rax)

    movq 32(%rsp), %rcx     # Ger rflags from stack
    movq %rcx, 136(%rax)
    
    movq 24(%rsp), %rcx     # Get CS from stack
    movq %rcx, 128(%rax)

    test $3, %rcx
    jne user_mode_interrupt

    # Stack layout right now:
    # 0(%rsp)  -> int_no
    # 8(%rsp)  -> err_code
    # 16(%rsp) -> rip
    # 24(%rsp) -> cs
    # 32(%rsp) -> rflags
    # We do not have user rsp and ss on the stack
    # because we're interrupting in ring 0
    # this means that our previous RSP can be
    # calculated at 40(%rsp) and SS we set 
    # manually
    # For my future self: it is at 40(%rsp) because 
    # that is quite literally the place it pointed to
    # last before the interrupt occured, which literally means
    # before CPU automatically pushed rip,cs,rflags and 
    # before I pushed int_no and err_code 
    
    movq 40(%rsp), %rcx 
    movq %rcx, 144(%rax)    # RSP
    movq $0x10, 152(%rax)   # SS for kernel is 0x10

    # What we need to do now have only the values 
    # we want as our interrupt frame on the stack
    # so time to pop 2 times to get int_no and err_no
    # then 3 times with irrelevant values (we already saved them)
    # and then finally push back 3 values, int_no, error_code and cr2

    popq %rax # thank you for your service as a pointer! 
    popq %rbx 
    popq %rcx # RIP is here
    popq %rcx # now CS 
    popq %rcx # now rflags 

    # Check stack alignment (System V ABI requirement)
    test $15, %rsp
    jnz will_be_aligned_after_push
    subq $8, %rsp

will_be_aligned_after_push:
    
    # The stack is now empty, we need to push back int_no and err_code 
    pushq %rbx # Push error_code 
    pushq %rax # Push int_no
    # Get cr2 for page faults:
    movq %cr2, %rax 
    pushq %rax

    movq %rsp, %rdi      # Pass stack pointer as first argument
    call isr_dispatch
    

user_mode_interrupt:
    call print_check 
