.section .text
.global _start
.type _start, @function

_start:
    # Set up stack
    mov $stack_top, %rsp

    # Clear direction flag
    cld

    # Call kernel main
    call kernel_main

    # Halt if kernel returns
    cli
1:  hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
.skip 16384  # 16 KB stack
stack_top:
