#ifndef __KERNEL_LOGGING_H
#define __KERNEL_LOGGING_H

#include <stdarg.h>

#define KPRINTF_BUF_SIZE 1024
// Function that formats the output string
int kvsprintf(char *buf, const char* __restrict, va_list args);

/* Kernel print function to terminal
__attribute__(format(printf,1,2)) tells the compiler
that we are dealing with a formatted function so the compiler
issues out warnings if incorrect format specifiers are used for certain arguments
example: kprintf("%c = %s", 'x', 8*5); - we're passing 8*5 and specifying format %s 
*/
__attribute__((format(printf, 1, 2)))
void kprintf(const char* __restrict, ...);


#endif
