.section .text

.extern isr_common_stub
.macro ISR_NOERR n
.global isr\n
isr\n:
    cli                 # Disable interrupts
    pushl $0            # Fake error code (0)
    pushl $\n           # Interrupt number (n)
    jmp isr_common_stub # Jump to common handler
.endm

.macro ISR_ERR n
.global isr\n
isr\n:
    cli                 # Disable interrupts
    pushl $\n           # Interrupt number (n), error code is already on the stack
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

.global isr_common_stub
isr_common_stub:
    pusha 
    movl %ds, %eax 
    pushl %eax
    movl %cr2, %eax
    pushl %eax 

    movw $0x10, %ax
    movw %ax, %ds 
    movw %ax, %es 
    movw %ax, %fs 
    movw %ax, %gs 
    
    pushl %esp
    call isr_dispatch       # Call the C function isr_dispatch(struct regs_t r*)
    addl $4, %esp           # Clean up stack after the call to isr_dispatch
    
    popl %ebx
    movw %bx, %ds
    movw %bx, %es
    movw %bx, %fs
    movw %bx, %gs

    popa
    addl $8, %esp    

    sti                     # Re-enable interrupts
    iret                    # Return from interrupt
