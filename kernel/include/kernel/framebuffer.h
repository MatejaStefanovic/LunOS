#ifndef __KERNEL_FRAMEBUFFER_H
#define __KERNEL_FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/limine_requests.h>
#include <kernel/tty_font.h>

struct limine_framebuffer *fb_get(void);

int fb_init(void);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_put_char(char c, uint32_t x, uint32_t y, uint32_t fg_color, uint32_t bg_color);
void fb_put_string(const char* str, uint32_t x,
        uint32_t y, uint32_t fg_color, uint32_t bg_color);
#endif
