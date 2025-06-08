#ifndef HALT_H
#define HALT_H

static inline void hcf(void) {
    for (;;)
        asm volatile("hlt");
}

#endif
