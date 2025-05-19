#ifndef __KERNEL_BUFFER_UTILS_H
#define __KERNEL_BUFFER_UTILS_H

#define BUFFER_SAFE_WRITE_CH(buf, len, max, ch) \
    (((len) < (max)) ? ((buf)[(len)++] = (ch), 1) : 0)

#define BUFFER_SAFE_WRITE_STR(buf, len, max, str)   \
    ({                                              \
        const char *s = (str);                      \
        int res = 1;                                \
                                                    \
        while(*s && (len) < (max)-1)                \
            (buf)[(len)++] = *s++;                  \
        if(*s != '\0' && (len) >= (max)-1)          \
            res = 0;                                \
        res;                                        \
    })
    
    
    

#endif
