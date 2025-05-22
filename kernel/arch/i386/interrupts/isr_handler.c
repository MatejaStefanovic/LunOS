#include <kernel/isr_handler.h>
#include <kernel/klogging.h>

void print_hex(uint32_t value) {
    // We'll print 8 digits for a 32-bit value
    char hex_digits[] = "0123456789abcdef";
    char buffer[9];  // To hold the hex string + null terminator

    buffer[8] = '\0';  // Null-terminate the string

    for (int i = 7; i >= 0; --i) {
        buffer[i] = hex_digits[value & 0xF];  // Get the last hex digit
        value >>= 4;  // Shift the value 4 bits to the right
    }

    kprintf("0x%s", buffer);  // Print the result, e.g., "0x00001a3f"
}

void isr0_divide_by_zero(){
    kprintf("EXCEPTION: Divide by zero");
    kprintf("\n");

    for(;;);
}

void isr_dispatch(struct regs_t *r){
    switch (r->int_no){
        // 0 - 32 - Exception handlers
        case 0: 
            isr0_divide_by_zero();
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14: // Page fault
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29: 
        case 30:
        case 31:
            kprintf("isr%d", (int)r->int_no);
            for(;;);
            break;

        // IRQ handlers
        case 33:
            break;
    }
}
