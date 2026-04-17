#include <stdbool.h>
#include <stddef.h>
#include <klib/string.h>

#include <kernel/tty.h>
#include <kernel/limine.h>
#include <kernel/framebuffer.h>
#include <kernel/halt.h>
#include <kernel/spinlock.h>

static uint16_t total_rows;
static uint16_t total_columns;
static uint16_t current_row;
static uint16_t current_column;
static uint32_t terminal_fg_color;
static uint32_t terminal_bg_color;
static struct term_cell_t terminal_buffer[240*135];

void get_screen_dimensions(void){
    struct limine_framebuffer *fb = fb_get();
    if(!fb)
        hcf();
    total_columns = fb->width / FONT_WIDTH;
    total_rows = fb->height / FONT_HEIGHT;
}

void terminal_initialize(uint32_t fg, uint32_t bg) {
    get_screen_dimensions();
    current_row = 0;
    current_column = 0;

    terminal_fg_color = fg;
    terminal_bg_color = bg;

    size_t buffer_size = total_rows * total_columns;
    
    for (size_t i = 0; i < buffer_size; i++) {
        terminal_buffer[i] = (struct term_cell_t){ .ch = ' ', .fg = fg, .bg = bg };
    }
    terminal_render();
}

void terminal_render(void) {
    for (size_t y = 0; y < total_rows; y++) {
        for (size_t x = 0; x < total_columns; x++) {
            const size_t index = y * total_columns + x;
            struct term_cell_t cell = terminal_buffer[index];
            
            fb_put_char(cell.ch, x * FONT_WIDTH, y * FONT_HEIGHT, cell.fg, cell.bg);
        }
    }
}

static void terminal_putentryat(struct term_cell_t c, size_t x, size_t y) {
    if (x >= total_columns || y >= total_rows) {
        return; 
    }
    const size_t index = y * total_columns + x;
    terminal_buffer[index] = c;
    fb_put_char(c.ch, x * FONT_WIDTH, y * FONT_HEIGHT, c.fg, c.bg);
}

void terminal_putchar(struct term_cell_t c) {
    if (c.ch == '\n') {
        terminal_newline();
        return;
    }
    if (c.ch == '\t') {
        current_column += TAB_WIDTH - (current_column % TAB_WIDTH);

        if (current_column >= total_columns) {
            terminal_newline();
        }
        return;
    }

    terminal_putentryat(c, current_column, current_row);
    
    if (++current_column >= total_columns) {
        terminal_newline();
    }
}

void terminal_newline(void) {
    current_column = 0;
    if (++current_row >= total_rows) {
        terminal_scroll();
        // Redner prints the whole buffer to the screen, we only want to start
        // doing that once we need to Scroll otherwise we'd needlessly 
        // go over the entire buffer every time we write a character
        terminal_render(); 
    }
}

// Scroll the terminal up by one line
void terminal_scroll(void) {
    // Move all lines up by one row
    memmove(
        terminal_buffer,
        terminal_buffer + total_columns,
        (total_rows - 1) * total_columns * sizeof(struct term_cell_t)
    );
    
    // Clear the last row
    for (size_t i = 0; i < total_columns; i++) {
        terminal_buffer[(total_rows - 1) * total_columns + i].ch = ' ';
    }
    
    current_row = total_rows - 1;
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        struct term_cell_t cell = (struct term_cell_t){ .ch = data[i], 
            .fg = terminal_fg_color, .bg = terminal_bg_color }; 
        terminal_putchar(cell);
    }
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void terminal_clear(void) {
    size_t buffer_size = total_rows * total_columns;
        
    for (size_t i = 0; i < buffer_size; i++) {
        terminal_buffer[i].ch = ' ';
        terminal_buffer[i].fg = 0xFFFFFF;
        terminal_buffer[i].bg = 0x000035;
    }
    
    current_row = 0;
    current_column = 0;
    terminal_render();
}

void terminal_setcolor(uint32_t fg_color, uint32_t bg_color) {
    terminal_fg_color = fg_color;
    terminal_bg_color = bg_color;
}
