#include <kernel/gdt_init.h>
#include <klib/string.h>

static struct global_descr_table gdt;
struct gdt_ptr gdtr;
static struct task_state_seg tss;

static void load_tss(void) {
    size_t addr = (size_t)&tss;

    gdt.tss_desc.limit_lower = sizeof(struct task_state_seg) - 1;
    gdt.tss_desc.base_lower = (uint16_t)(addr & 0xFFFF);
    gdt.tss_desc.base_middle = (uint8_t)((addr >> 16) & 0xFF);
    gdt.tss_desc.access = 0x89;
    gdt.tss_desc.granularity = 0;
    gdt.tss_desc.base_high = (uint8_t)((addr >> 24) & 0xFF);
    gdt.tss_desc.base_upper = (uint32_t)(addr >> 32);
    gdt.tss_desc.reserved = 0;
    
    // 5*8 is because this is the fifth entry
    asm volatile("ltr %0" : : "r"((uint16_t)(5 * 8)) : "memory");
}

static void set_gdt_entry(int index, uint8_t access, uint8_t granularity){
    gdt.entries[index].limit_lower = 0;
    gdt.entries[index].base_lower = 0; 
    gdt.entries[index].base_middle = 0;
    gdt.entries[index].access = access;
    gdt.entries[index].granularity = granularity;
    gdt.entries[index].base_higher = 0;
}

void init_gdt(void){
    memset(&gdt, 0, sizeof(gdt));
    
    set_gdt_entry(0, 0, 0);
    set_gdt_entry(1, 0x9A, 0x20);
    set_gdt_entry(2, 0x92, 0);
    set_gdt_entry(3, 0xFA, 0x20);
    set_gdt_entry(4, 0xF2, 0);

    gdtr.limit = sizeof(struct global_descr_table) - 1;
    gdtr.base = (uint64_t)&gdt;

    reload_gdt();

    memset(&tss, 0, sizeof(struct task_state_seg));
    load_tss();
}
