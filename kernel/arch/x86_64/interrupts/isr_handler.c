#include <kernel/isr_handler.h>
#include <kernel/klogging.h>

void decode_page_fault_error(uint64_t err_code) {
    kprintf("Error code: 0x%lx\n", err_code);

    if (err_code & 0x1)
        kprintf(" - Protection Violation (page present) -\n");
    else
        kprintf(" - Page Not Present -\n");

    if (err_code & 0x2)
        kprintf(" - Fault caused by Write -\n");
    else
        kprintf(" - Fault caused by Read -\n");

    if (err_code & 0x4)
        kprintf(" - Fault in User Mode -\n");
    else
        kprintf(" - Fault in Supervisor Mode -\n");

    if (err_code & 0x8)
        kprintf(" - Reserved Bit Violation -\n");

    if (err_code & 0x10)
        kprintf(" - Instruction Fetch Fault -\n");
}

void isr0_divide_by_zero(struct regs_t *r){
    kprintf("EXCEPTION: Divide by zero\n");
    kprintf("Division happened at address: 0x%lx", r->rip);
    kprintf("\n");

    for(;;);
}

void isr14_page_fault(struct regs_t *r){
    kprintf("ERROR: page fault occurred at address: %lx\n", r->cr2);
    decode_page_fault_error(r->err_code);
}

void isr_reserved(){
    kprintf("ISR is reserved by INTEL!? How are we even here\n");
}
void isr_dispatch(struct regs_t *r){
    switch (r->int_no){
        // 0 - 32 - Exception handlers
        case 0: 
            isr0_divide_by_zero(r);
            break;
        case 14: // Page fault
            isr14_page_fault(r);
            break;
        
        case 15:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 31:
           isr_reserved();
           break;
        default:
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
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 28:
        case 29: 
        case 30:
            kprintf("isr%lu", r->int_no);
            for(;;);
            break;

        // IRQ handlers
        case 33:
            break;
    }
}
