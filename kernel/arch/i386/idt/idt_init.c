#include <kernel/idt_init.h>

struct idt_entry_t idt[IDT_SIZE];
struct idt_ptr idtr;

void create_gate_entry(uint8_t entry_index, isr_t handler, uint16_t seg_selector, uint8_t flags){
    uint32_t handler_addr = (uint32_t)handler;

    idt[num].offset_lower = handler_addr & 0xFFFF;
    idt[num].offset_higher = (handler_addr >> 16) & 0xFFFF;
    idt[num].seg_selector = seg_selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
    
}

void init_idt(){
   
    idtr.limit = sizeof(idt)-1;
    idtr.base = (uint32_t)&idt;

    //setIdt(idtr.limit, idtr.base);
}
