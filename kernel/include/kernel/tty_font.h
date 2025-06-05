#ifndef __KERNEL_TTY_FONT_H
#define __KERNEL_TTY_FONT_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

// Get font data for a character
const uint8_t* font_get_char(char c);

#endif
