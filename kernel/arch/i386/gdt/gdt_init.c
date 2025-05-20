#include <kernel/gdt_init.h>

struct gdt_entry gdt[NUM_OF_ENTRIES];
struct tss_entry tss;
void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity){
    gdt[index].limit_lower  = limit & 0xFFFF; // Set bits 0-15
    gdt[index].base_lower   = base & 0xFFFF; // Set bits 0-15
    gdt[index].base_middle  = (base >> 16) & 0xFF; // Set bits 16–23 
    gdt[index].access       = access; // Set the access byte
    gdt[index].granularity  = ((limit >> 16) & 0x0F) | (granularity & 0xF0); // Set bits 17-20 for limit 
    gdt[index].base_high    = (base >> 24) & 0xFF; // Set bits 24-31
}
void create_tss_entry(){
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(struct tss_entry);
    // TSS is a structured object, not just a range of memory like
    // other entries so we are required to put the actual address of the TSS
    // as base and limit as the size of our tss struct which is 104 bytes
    set_gdt_entry(GDT_ENTRY_TSS, base, limit, 0x89, 0x00);
}
void init_gdt(){
    set_gdt_entry(GDT_ENTRY_NULL, 0, 0, 0, 0);
    /* base = 0, limit = 0xFFFFF, 
     * access is 0x9A = 10011010 which means:
     * P = 1, DPL = 00 (Ring 0, 11 is Ring 3)
     * S = 1 (code segment, 0 for data)
     * and the remaining 0xA means executable and readable
     * in other examples 0x2 means writeable
     * granularity 0xCF = 11001111
     * G = 1 (granularity bit)
     * D/B = 1 (segment is treated as 32 bit)
     * 00 is reserved and unused
     * 0xF is used for the final 4 bits for limit,
     * tells the total size of the segment, in our case
     * a total of 20 bits */
    set_gdt_entry(GDT_ENTRY_KCODE, 0, 0xFFFFF, 0x9A, 0xCF);
    set_gdt_entry(GDT_ENTRY_KDATA, 0, 0xFFFFF, 0x92, 0xCF);
    set_gdt_entry(GDT_ENTRY_UCODE, 0, 0xFFFFF, 0xFA, 0xCF);
    set_gdt_entry(GDT_ENTRY_UDATA, 0, 0xFFFFF, 0xF2, 0xCF);
    create_tss_entry();  


    struct gdt_ptr gdtr;
    gdtr.limit = sizeof(struct gdt_entry) * NUM_OF_ENTRIES - 1; 
    gdtr.base = (uint32_t)&gdt; // address of the first entry which is null segment

    /* 
     * setGdt is an asm function located in gdt.s file used to load GDT into 
     * the gdtr (gdt register) using lgdt instruction
    */ 
    setGdt(gdtr.limit, gdtr.base);

    /* 
     * reloadSegments is an asm function located in gdt.s file used to load
     * segment selectors so that the CPU can deal with GDT 
    */
    reloadSegments();
}

