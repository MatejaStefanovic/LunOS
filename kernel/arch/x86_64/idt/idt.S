.section .data
idtr:
    .word 0     # 16-bit limit
    .quad 0     # 64-bit base address

.section .text
.globl setIdt
setIdt:
    # x86_64 calling convention: first arg in %rdi, second in %rsi
    movw %di, idtr          # limit (lower 16 bits of %rdi) 
    movq %rsi, idtr+2       # base address (64-bit)
    lidt idtr
    sti                     # Enable interrupts after IDT is loaded
    ret
