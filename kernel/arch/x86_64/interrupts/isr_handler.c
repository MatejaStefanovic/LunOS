#include <kernel/isr_handler.h>
#include <kernel/klogging.h>
#include <kernel/halt.h>
#include <kernel/memmgr.h>
#include <kernel/scheduler.h>
#include <kernel/apic.h>

static void decode_page_fault_error(uint64_t err_code) {
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

static void isr0_divide_by_zero(void){
    kprintf("EXCEPTION: Divide by zero\n");
    build_iretq_frame(&get_current_task()->cpu_context); 
}

static void isr14_page_fault(uint64_t cr2, uint64_t err_code){
    if(cr2 >= KERNEL_SPACE_START){
        KERROR("Page fault occurred at address: %lx\n", cr2);
        decode_page_fault_error(err_code);
        kprintf("This page fault occured in kernel space.. Time to panic :d\n");
        hcf();
    }
    KERROR("Page fault occurred at address: %lx\n", cr2);
    decode_page_fault_error(err_code);
    hcf();
    //mm_page_fault_handler(cr2, fr->err_code);
}


static void isr_reserved(void){
    kprintf("ISR is reserved by INTEL!? How are we even here\n");
}

void print_check(void){
    kprintf("THEORY CONFIRMED BOYS!\n");
    while(1);
}

void isr_dispatch(struct interrupt_frame *fr){
    switch (fr->int_no){
        // 0 - 32 - Exception handlers
        case 0: 
            isr0_divide_by_zero();
            break;
        case 14: // Page fault
            isr14_page_fault(fr->cr2, fr->err_code);
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
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 28:
        case 29: 
        case 30:
            kprintf("isr%lu", fr->int_no);
            hcf();
            break;
        case 13:
            KERROR("GPF, Error Code: 0x%lx\n",fr->err_code);
            break;
        // IRQ handlers
        case 33:
            break;
        case 64:
            apic_timer_handler();
            break;
    }
}
