#ifndef __KERNEL_LOGGING_H
#define __KERNEL_LOGGING_H

#include <stdarg.h>
#include <stdint.h>
#include <kernel/tty.h>
#include <kernel/compiler.h>

#define KPRINTF_BUF_SIZE 1024
#define MAX_INT_DIGITS 21
#define MAX_HEX_DIGITS 17

// Function that formats the output string
int kvsprintf(char *buf, const char* __restrict, va_list args);

/* Kernel print function to terminal
__attribute__(format(printf,1,2)) tells the compiler
that we are dealing with a formatted function so the compiler
issues out warnings if incorrect format specifiers are used for certain arguments
example: kprintf("%c = %s", 'x', 8*5); - we're passing 8*5 and specifying format %s 
*/

_print_fmt(1, 2)
void kprintf(const char* __restrict, ...);

_print_fmt(1, 2)
void KSUCCESS(const char* __restrict, ...);

_print_fmt(1, 2)
void KWARN(const char* __restrict, ...);

_print_fmt(1, 2)
void KERROR(const char* __restrict, ...);

#endif
