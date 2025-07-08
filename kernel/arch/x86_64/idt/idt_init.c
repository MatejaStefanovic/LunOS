#include <kernel/idt_init.h>
#include <kernel/pic.h>
#include <kernel/isr_handler.h>
#include <kernel/klogging.h>
#include <klib/string.h>

static struct idt_entry_t idt[IDT_SIZE];
static struct idt_ptr idtr;

void create_gate_entry(uint8_t entry_index, isr_t handler, uint16_t seg_selector, uint8_t attributes){
    uint64_t handler_addr = (uint64_t)handler;
    
    idt[entry_index].isr_low = handler_addr & 0xFFFF;
    idt[entry_index].isr_mid = (handler_addr >> 16) & 0xFFFF;
    idt[entry_index].isr_high = (handler_addr >> 32) & 0xFFFFFFFF;
    idt[entry_index].kernel_cs = seg_selector;
    idt[entry_index].ist = 0;  // IST = 0 as we discussed
    idt[entry_index].attributes = attributes;
    idt[entry_index].reserved = 0;
}

void init_idt(void){ 
    kprintf("Initializing Interrupt table...\n");
    
    idtr.limit = sizeof(idt)-1;
    idtr.base = (uint64_t)&idt;
   
    memset(&idt, 0, sizeof(struct idt_entry_t)*256);
    
    pic_remap(0x20, 0x28); // remapped to 32 and 40 for slave and master PIC
    disable_pic();

    create_gate_entry(0, isr0, 0x08, 0x8E);
    create_gate_entry(1, isr1, 0x08, 0x8E);
    create_gate_entry(2, isr2, 0x08, 0x8E);
    create_gate_entry(3, isr3, 0x08, 0x8E);
    create_gate_entry(4, isr4, 0x08, 0x8E);
    create_gate_entry(5, isr5, 0x08, 0x8E);
    create_gate_entry(6, isr6, 0x08, 0x8E);
    create_gate_entry(7, isr7, 0x08, 0x8E);
    create_gate_entry(8, isr8, 0x08, 0x8E);
    create_gate_entry(9, isr9, 0x08, 0x8E);
    create_gate_entry(10, isr10, 0x08, 0x8E);
    create_gate_entry(11, isr11, 0x08, 0x8E);
    create_gate_entry(12, isr12, 0x08, 0x8E);
    create_gate_entry(13, isr13, 0x08, 0x8E);
    create_gate_entry(14, isr14, 0x08, 0x8E);
    create_gate_entry(15, isr15, 0x08, 0x8E);
    create_gate_entry(16, isr16, 0x08, 0x8E);
    create_gate_entry(17, isr17, 0x08, 0x8E);
    create_gate_entry(18, isr18, 0x08, 0x8E);
    create_gate_entry(19, isr19, 0x08, 0x8E);
    create_gate_entry(20, isr20, 0x08, 0x8E);
    create_gate_entry(21, isr21, 0x08, 0x8E);
    create_gate_entry(22, isr22, 0x08, 0x8E);
    create_gate_entry(23, isr23, 0x08, 0x8E);
    create_gate_entry(24, isr24, 0x08, 0x8E);
    create_gate_entry(25, isr25, 0x08, 0x8E);
    create_gate_entry(26, isr26, 0x08, 0x8E);
    create_gate_entry(27, isr27, 0x08, 0x8E);
    create_gate_entry(28, isr28, 0x08, 0x8E);
    create_gate_entry(29, isr29, 0x08, 0x8E);
    create_gate_entry(30, isr30, 0x08, 0x8E);
    create_gate_entry(31, isr31, 0x08, 0x8E);
    
    setIdt(idtr.limit, idtr.base);

    KSUCCESS("Interrupt setup was successfull\n");  
}

void reload_idt(void) {
    setIdt(idtr.limit, idtr.base);
}

