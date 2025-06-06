#ifndef __KERNEL_LOGGING_H
#define __KERNEL_LOGGING_H

#include <stdarg.h>
#include <stdint.h>
#include <kernel/tty.h>

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
__attribute__((format(printf, 1, 2)))
void kprintf(const char* __restrict, ...);


#define KSUCCESS(fmt, ...) \
    do { \
        kprintf("["); \
        terminal_setcolor(0x00FF00, 0x000035); \
        kprintf("OK"); \
        terminal_setcolor(0xFFFFFF, 0x000035); \
        kprintf("] "); \
        kprintf(fmt, ##__VA_ARGS__); \
    } while (0)

#define KWARN(fmt, ...) \
    do { \
        kprintf("["); \
        terminal_setcolor(0xFFFF00, 0x000035); \
        kprintf("WARNING"); \
        terminal_setcolor(0xFFFFFF, 0x000035); \
        kprintf("] "); \
        kprintf(fmt, ##__VA_ARGS__); \
    } while (0)

#define KERROR(fmt, ...) \
    do { \
        kprintf("["); \
        terminal_setcolor(0xFF0000, 0x000035); \
        kprintf("ERROR"); \
        terminal_setcolor(0xFFFFFF, 0x000035); \
        kprintf("] "); \
        kprintf(fmt, ##__VA_ARGS__); \
    } while (0)

#endif
