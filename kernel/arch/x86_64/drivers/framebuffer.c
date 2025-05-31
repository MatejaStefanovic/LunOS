#include <kernel/framebuffer.h>


static struct limine_framebuffer *current_fb = NULL;

int fb_init(void) {
    struct limine_framebuffer_request *fb_req = get_framebuffer_request();

    if (fb_req->response == NULL || fb_req->response->framebuffer_count < 1) {
        return 0;
    }

    current_fb = fb_req->response->framebuffers[0];
    return 1;
}

struct limine_framebuffer* fb_get(void) {
    return current_fb;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!current_fb) return;

    volatile uint32_t *fb_ptr = current_fb->address;
    if (x < current_fb->width && y < current_fb->height) {
        fb_ptr[y * (current_fb->pitch / 4) + x] = color;
    }
}

void fb_clear(uint32_t color) {
    if (!current_fb) return;

    volatile uint32_t *fb_ptr = current_fb->address;
    for (uint32_t y = 0; y < current_fb->height; y++) {
        for (uint32_t x = 0; x < current_fb->width; x++) {
            fb_ptr[y * (current_fb->pitch / 4) + x] = color;
        }
    }
}

void fb_draw_diagonal(uint32_t color, size_t length) {
    if (!current_fb) return;

    volatile uint32_t *fb_ptr = current_fb->address;
    for (size_t i = 0; i < length && i < current_fb->width && i < current_fb->height; i++) {
        fb_ptr[i * (current_fb->pitch / 4) + i] = color;
    }
}
