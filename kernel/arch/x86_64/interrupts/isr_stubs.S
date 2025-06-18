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

.global isr_common_stub
isr_common_stub:
    # Save all general purpose registers
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    
    # Save CR2 for page fault handling
    movq %cr2, %rax
    pushq %rax
    
    # Call C handler - first parameter is in %rdi in x86_64 calling convention
    movq %rsp, %rdi      # Pass stack pointer as first argument
    call isr_dispatch
    
    # Restore CR2 (discard the value)
    popq %rax
    
    # Restore general purpose registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    
    addq $16, %rsp       # Clean up error code and interrupt number (2 * 8 bytes)
    iretq                # 64-bit interrupt return
