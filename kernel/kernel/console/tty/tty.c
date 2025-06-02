#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/limine.h>
#include <kernel/framebuffer.h>
#include "tty.h"

static uint16_t total_rows;
static uint16_t total_columns;
static uint16_t current_row;
static uint16_t current_column;
static uint32_t terminal_fg_color;
static uint32_t terminal_bg_color;
static char terminal_buffer[240*135];

void get_screen_dimensions(){
    struct limine_framebuffer *fb = fb_get();
    if(!fb)
        for(;;);
    total_columns = fb->width / FONT_WIDTH;
    total_rows = fb->height / FONT_HEIGHT;
    total_rows -= 10; 
}

void terminal_initialize(void) {
    get_screen_dimensions();
    current_row = 0;
    current_column = 0;
    terminal_fg_color = 0xFFFFFF; // White
    terminal_bg_color = 0x000000; // Black
    
    size_t buffer_size = total_rows * total_columns;
    
    // Initialize buffer with spaces
    for (size_t i = 0; i < buffer_size; i++) {
        terminal_buffer[i] = ' ';
    }
}

// Render the entire buffer to the framebuffer
void terminal_render(void) {
    for (size_t y = 0; y < total_rows; y++) {
        for (size_t x = 0; x < total_columns; x++) {
            const size_t index = y * total_columns + x;
            char ch = terminal_buffer[index];
            
            fb_put_char(ch, x * FONT_WIDTH, y * FONT_HEIGHT, terminal_fg_color, terminal_bg_color);
        }
    }
}

// Put a character at a specific position in the buffer
void terminal_putentryat(char c, size_t x, size_t y) {
    if (x >= total_columns || y >= total_rows) {
        return; // Bounds check
    }
    const size_t index = y * total_columns + x;
    terminal_buffer[index] = c;
    fb_put_char(c, x * FONT_WIDTH, y * FONT_HEIGHT, terminal_fg_color, terminal_bg_color);
}

// Put a character at the current cursor position
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_newline();
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
        (total_rows - 1) * total_columns * sizeof(char)
    );
    
    // Clear the last row
    for (size_t i = 0; i < total_columns; i++) {
        terminal_buffer[(total_rows - 1) * total_columns + i] = ' ';
    }
    
    current_row = total_rows - 1;
}

// Write a string to the terminal
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
    // Only render once after writing all characters
}

// Write a null-terminated string
void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

// Clear the entire terminal
void terminal_clear(void) {
    size_t buffer_size = total_rows * total_columns;
    
    for (size_t i = 0; i < buffer_size; i++) {
        terminal_buffer[i] = ' ';
    }
    
    current_row = 0;
    current_column = 0;
    terminal_render();
}

// Set terminal colors
void terminal_setcolor(uint32_t fg_color, uint32_t bg_color) {
    terminal_fg_color = fg_color;
    terminal_bg_color = bg_color;
}
