#include <kernel/isr_dispatcher.h>

void isr_dispatch(struct regs_t *r){
    switch (r->int_no){
        // 0 - 32 - Exception handlers
        case 0: 
            handle_divide_by_zero(r);
            break;

        case 14: // Page fault
            break;

        // IRQ handlers
        case 33:
            break;
    }
}
