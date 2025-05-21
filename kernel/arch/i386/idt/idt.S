.section .data
idtr:
    .word 0
    .long 0

.section .text
.globl setIdt

setIdt:
    movw 4(%esp), %ax //get limit arg from stack and put it into ax
    movw %ax, idtr
    movl 8(%esp), %eax
    movl %eax, idtr+2
    lidt idtr
    ret

