.section .text

    .extern isr_common_stub

    .macro ISR_NOERR n
    .global isr\n
    isr\n:
        cli                 # Disable interrupts
        pushl $0            # Fake error code (0)
        pushl $ \n          # Interrupt number (n)
        jmp isr_common_stub # Jump to common handler
    .endm

    .macro ISR_ERR n
    .global isr\n
    isr\n:
        cli                 # Disable interrupts
        pushl $ \n          # Interrupt number (n), error code is already on the stack
        jmp isr_common_stub # Jump to common handler
    .endm


    # Define stubs for exceptions (0-31)
    ISR_NOERR 0    # Divide-by-zero
    ISR_NOERR 1    # Debug
    ISR_NOERR 2    # NMI
    ISR_NOERR 3    # Breakpoint
    ISR_NOERR 4    # Overflow
    ISR_ERR 14     # Page Fault (has real error code)

.global isr_common_stub
isr_common_stub:
    pushl %eax               # Save eax
    pushl %ecx               # Save ecx
    pushl %edx               # Save edx
    pushl %ebx               # Save ebx
    pushl %esp               # Save esp
    pushl %ebp               # Save ebp
    pushl %esi               # Save esi
    pushl %edi               # Save edi

    pushl %ds                # Save ds
    pushl %es                # Save es
    pushl %fs                # Save fs
    pushl %gs                # Save gs

    movl $0x10, %eax         # Load kernel data segment selector (from GDT) into eax
    movl %eax, %ds           # Set ds to 0x10
    movl %eax, %es           # Set es to 0x10
    movl %eax, %fs           # Set fs to 0x10
    movl %eax, %gs           # Set gs to 0x10

    movl %esp, %eax         # Pass pointer to regs (current stack pointer)
    pushl %eax
    call isr_dispatch       # Call the C function isr_dispatch(struct regs*)
    addl $4, %esp           # Clean up stack after the call to isr_dispatch

    popl %gs                # Restore gs
    popl %fs                # Restore fs
    popl %es                # Restore es
    popl %ds                # Restore ds

    popl %edi               # Restore edi
    popl %esi               # Restore esi
    popl %ebp               # Restore ebp
    popl %esp               # Restore esp
    popl %ebx               # Restore ebx
    popl %edx               # Restore edx
    popl %ecx               # Restore ecx
    popl %eax               # Restore eax

    addl $8, %esp           # Clean up the error code and interrupt number
    sti                     # Re-enable interrupts
    iret                    # Return from interrupt
