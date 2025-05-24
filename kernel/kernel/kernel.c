#include <kernel/gdt_init.h>
#include <kernel/tty.h>
#include <kernel/klogging.h>
#include <kernel/idt_init.h>
#include <kernel/isr_handler.h>
#include <kernel/paging.h>
#include <kernel/multiboot.h>

#define KERNEL_VIRTUAL_BASE 0xC0000000
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))
void kernel_main(unsigned long magic, unsigned long addr) {
    init_gdt(); // Set up GDT and reload segment buffers 
    init_idt(); // Set up IDT
    terminal_initialize(); // Set up VGA for text output 
    
    kprintf("magic = 0x%x\n addr = 0x%x\n", magic, addr);
    kprintf("I started this gangsta shit?! And this the motherfucking chance I get?\n");
    kprintf("                                   HELLO\n");
    kprintf("What happened in Monte Carlo happened...\n");
    kprintf("What happened in Barcelona happened...\n");
    kprintf("What happened in Madrdid happened and we are here, ");
    kprintf("we're in Rome...\n");
    
    kprintf("addr = %x\n", addr);
    addr+= KERNEL_VIRTUAL_BASE;
    
    multiboot_info_t *mbi;
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    kprintf ("flags = 0x%x\n", (unsigned) mbi->flags);
     
    if (CHECK_FLAG (mbi->flags, 0))
        kprintf ("mem_lower = %luKB, mem_upper = %luKB\n",
            (unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

    if (CHECK_FLAG (mbi->flags, 6))
    {
        multiboot_memory_map_t *mmap;
      
        kprintf ("mmap_addr = 0x%x, mmap_length = 0x%x\n",
              (unsigned) mbi->mmap_addr + KERNEL_VIRTUAL_BASE, 
              (unsigned) mbi->mmap_length + KERNEL_VIRTUAL_BASE);

        unsigned long mmap_phys_start = mbi->mmap_addr;
        unsigned long mmap_phys_end = mbi->mmap_addr + mbi->mmap_length;
        unsigned long current_phys = mmap_phys_start;

        while (current_phys < mmap_phys_end) {
            multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)(current_phys + KERNEL_VIRTUAL_BASE);
        
            kprintf (" size = 0x%x, base_addr = 0x%x%x,"
                " length = 0x%x%x, type = 0x%x\n",
                (unsigned) mmap->size,
                (unsigned) (mmap->addr >> 32),
                (unsigned) (mmap->addr & 0xffffffff),
                (unsigned) (mmap->len >> 32),
                (unsigned) (mmap->len & 0xffffffff),
                (unsigned) mmap->type);
            current_phys += mmap->size + sizeof(mmap->size);
        }
    }
}
