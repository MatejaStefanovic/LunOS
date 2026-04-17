#ifndef __KERNEL_LIB_STRING_H
#define __KERNEL_LIB_STRING_H

#include <stddef.h>

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
char *kstrndup(const char *str, size_t n);
int strcmp(const char *str1, const char *str2);
char *strchr(const char *str, int ch);
char *kstrtok_r(char *str, const char *delim, char **saveptr);

#endif
