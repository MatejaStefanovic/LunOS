#ifndef _KERNEL_LOGGING_H
#define _KERNEL_LOGGING_H

#include <stdarg.h>

#define KPRINTF_BUF_SIZE 1024
// Function that formats the output string
int kvsprintf(char *, const char* __restrict, va_list);
// Kernel print function to terminal
void kprintf(const char* __restrict, ...);


#endif
