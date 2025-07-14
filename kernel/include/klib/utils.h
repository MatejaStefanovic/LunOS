#ifndef __KERNEL_LIB_UTILS_H
#define __KERNEL_LIB_UTILS_H

#include <stdint.h>

void reverse_str(char *str);
int ui64_to_hex_str(uint64_t val, char *str);
int ul_to_str(unsigned long value, char *str);

#endif
