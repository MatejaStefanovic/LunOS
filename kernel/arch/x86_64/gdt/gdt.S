.section .text
.global reload_gdt
.type reload_gdt, @function

reload_gdt:
    lgdt (gdtr)

    pushq $0x08
    lea 1f(%rip), %rax
    pushq %rax
    lretq                        # Far return to set CS

1:
    mov $0x10, %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %ss
    mov %eax, %fs
    mov %eax, %gs

    ret
