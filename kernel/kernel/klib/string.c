#include <klib/string.h>
#include <kernel/pmm.h>

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

char *kstrndup(const char *str, size_t n){
    if(!str)
        return NULL;
    
    char *tmp = kmalloc(n+1); // +1 for \0
    if(!tmp)
        return NULL;

    size_t i;
    for(i = 0; i < n && *str != '\0'; i++)
        tmp[i] = str[i];

    tmp[i] = '\0';
    return tmp;
}

int strcmp(const char *str1, const char *str2){
    const unsigned char *s1 = (unsigned char *) str1;
    const unsigned char *s2 = (unsigned char *) str2;

    while(*s1 == *s2 && *s1){
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

char *strchr(const char *str, int chr){
    char ch = (char)chr;
    
    while(1) {
        if (*str == ch) return (char *)str;
        if (*str == '\0') return NULL;
        str++;
    }
}

char *kstrtok_r(char *str, const char *delim, char **saveptr){
    // First call, subsequent calls are null
    if(str)
        *saveptr = str;
    else if (!*saveptr || **saveptr == '\0')
        return NULL;

    char *token = *saveptr;
    while (*token && !strchr(delim, *token)) {
        token++;
    }

    char *result = *saveptr;

    // End of original string
    if (*token == '\0') {
        *saveptr = token;
        return result;
    }
    
    // We found a delimiter
    *token = '\0';
    *saveptr = token + 1;
 
    return result; 
}

