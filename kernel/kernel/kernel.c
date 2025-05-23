#include <kernel/gdt_init.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>
#include <kernel/paging.h>
#include <kernel/multiboot2.h>

#define KERNEL_VIRTUAL_BASE 0xC0000000

extern char _kernel_start_virt[];
extern char _kernel_start_phys[];
extern char _kernel_end_phys[];
extern char _kernel_end_virt[];

void kernel_main(multiboot_uint32_t addr) {
    init_gdt(); // Set up GDT and reload segment buffers 
    init_idt(); // Set up IDT
    terminal_initialize(); // Set up VGA for text output  
                           
    /*
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO\n");
    kprintf("What happened in Monte Carlo happened...\n");
    kprintf("What happened in Barcelona happened...\n");
    kprintf("What happened in Madrdid happened and we are here, ");
    kprintf("we're in Rome...");
    */
    kprintf("kern_start phys= %x\n", _kernel_start_phys);
    kprintf("kern_end phys= %x\n", _kernel_end_phys);
    kprintf("kern_start virt= %x\n", _kernel_start_virt);
    kprintf("kern_end virt = %x\n", _kernel_end_virt);
    kprintf("info addres from grub = %x\n", KERNEL_VIRTUAL_BASE + addr);

    addr += KERNEL_VIRTUAL_BASE;
    if (addr & 7)
    {
        kprintf ("Unaligned mbi: %x\n", addr);
        return;
    }
    
    struct multiboot_tag *tag;
    uint32_t size = *(uint32_t *) addr;
    kprintf ("Announced mbi size %x\n", size);

    for (tag = (struct multiboot_tag *) (addr + 8);
        tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag 
                                       + ((tag->size + 7) & ~7)))
    {
        kprintf ("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);
        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_MMAP: {
            multiboot_memory_map_t *mmap;

            kprintf ("mmap\n");
      
            for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
                (multiboot_uint8_t *) mmap < (multiboot_uint8_t *) tag + tag->size;
                mmap = (multiboot_memory_map_t *) 
                ((unsigned long) mmap + ((struct multiboot_tag_mmap *) tag)->entry_size))
                {
                kprintf ("base_addr = 0x%x%x,"
                      " length = 0x%x%x, type = 0x%x\n",
                      (unsigned) (mmap->addr >> 32),
                      (unsigned) (mmap->addr & 0xffffffff),
                      (unsigned) (mmap->len >> 32),
                      (unsigned) (mmap->len & 0xffffffff),
                      (unsigned) mmap->type);
                }
            }
            break; 
        }
    }
}
