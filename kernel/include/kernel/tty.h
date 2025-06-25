#ifndef __KERNEL_TTY_H
#define __KERNEL_TTY_H

#define TAB_WIDTH 8
#include <stdint.h>
#include <stddef.h>

struct term_cell_t {
    char ch;
    uint32_t fg;
    uint32_t bg;
};

void get_screen_dimensions();
void terminal_initialize(uint32_t fg, uint32_t bg);
void terminal_putchar(struct term_cell_t c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_scroll(void);
void terminal_newline(void);
void terminal_clear(void);
void terminal_setcolor(uint32_t fg_color, uint32_t bg_color);
void terminal_render(void);

#endif
