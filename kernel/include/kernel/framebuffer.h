#ifndef __KERNEL_FRAMEBUFFER_H
#define __KERNEL_FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/limine_requests.h>

struct limine_framebuffer *fb_get(void);

int fb_init(void);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_diagonal(uint32_t color, size_t length);
#endif
