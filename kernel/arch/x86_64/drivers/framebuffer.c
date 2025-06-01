#include <kernel/framebuffer.h>
#include <kernel/tty_font.h>

static struct limine_framebuffer *primary_fb = NULL;

int fb_init(void) {
    struct limine_framebuffer_request *fb_req = get_framebuffer_request();

    if (fb_req->response == NULL || fb_req->response->framebuffer_count < 1) {
        return 0;
    }

    primary_fb = fb_req->response->framebuffers[0];
    return 1;
}

struct limine_framebuffer* fb_get(void) {
    return primary_fb;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!primary_fb) return;

    volatile uint32_t *fb_ptr = primary_fb->address;
    if (x < primary_fb->width && y < primary_fb->height) {
        fb_ptr[y * (primary_fb->pitch / 4) + x] = color;
    }
}

void fb_clear(uint32_t color) {
    if (!primary_fb) return;

    volatile uint32_t *fb_ptr = primary_fb->address;
    for (uint32_t y = 0; y < primary_fb->height; y++) {
        for (uint32_t x = 0; x < primary_fb->width; x++) {
            fb_ptr[y * (primary_fb->pitch / 4) + x] = color;
        }
    }
}

void fb_put_char(char c, uint32_t x, uint32_t y, uint32_t fg_color, uint32_t bg_color) {
    if (!primary_fb) return;

    const uint8_t *char_bitmap = font_get_char(c);
    volatile uint32_t *fb_ptr = primary_fb->address;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte = char_bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint32_t pixel_x = x + col;
            uint32_t pixel_y = y + row;

            if (pixel_x < primary_fb->width && pixel_y < primary_fb->height) {
                uint32_t color = (byte & (0x80 >> col)) ? fg_color : bg_color;
                fb_ptr[pixel_y * (primary_fb->pitch / 4) + pixel_x] = color;
            }
        }
    }
}
void fb_put_string(const char* str, uint32_t x,
        uint32_t y, uint32_t fg_color, uint32_t bg_color) {

    uint32_t current_x = x;

    while (*str) {
        if (*str == '\n') {
            // New line
            current_x = x;
            y += FONT_HEIGHT;
        } else {
            fb_put_char(*str, current_x, y, fg_color, bg_color);
            current_x += FONT_WIDTH;
        }
        str++;
    }
}
