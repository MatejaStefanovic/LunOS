#include <kernel/klogging.h>
#include <kernel/tty.h>
#include <kernel/bufferutils.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MAX_INT_DIGITS 12


int kvsprintf(char *buf, const char* restrict format, va_list args){
    int len = 0;
    while(*format){
        char curr_ch = *format++;
        
        // Not format specified -> add to buffer
        if(curr_ch != '%'){
            if(!BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, curr_ch)) 
                return -EOVERFLOW;
                            
            continue;
        }
        // Handle %% case
        if(*format == '%'){
            if(!BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, curr_ch)) 
                return -EOVERFLOW;
            
            ++format; // Necessary to go over the second % to not include it twice
            continue;
        }

        curr_ch = *format++;
        switch(curr_ch){
            case 'd': {
                int val = va_arg(args, int);
                char num_buf[MAX_INT_DIGITS]; // Enough digits for 32 bit int
                int int_str_len = itoa(val, num_buf);

                for(int i = 0; i < int_str_len; ++i){        
                    if(!BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, num_buf[i])) 
                        return -EOVERFLOW;
                }
                
                break;
            }
            case 's': {
                char *str = va_arg(args, char*); 
                if(!BUFFER_SAFE_WRITE_STR(buf,len, KPRINTF_BUF_SIZE, str))
                    return -EOVERFLOW;
                
                break;
            }
            case 'c': {
                /* Smaller types promote to int when there is an unknown
                 * amount of variables therefore it is needed to look for int */
                char ch = va_arg(args, int); 
                if(!BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, ch))
                    return -EOVERFLOW;
                
                break;
            }
            default: { //Unknown format specifier will just be printed out  
                if(!BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, '%') ||
                    !BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, curr_ch))
                    return -EOVERFLOW;
                
                break;
            }
        }
    }
    return len;
}

void kprintf(const char* restrict format, ...){
    char buf[KPRINTF_BUF_SIZE];
     
    va_list args;
    va_start(args, format);
    
    int out_len = kvsprintf(buf, format, args);

    va_end(args);

    if(out_len < 0){
        terminal_writestring("Error: Buffer overflow - data exceeds the allowed buffer size of 1024 bytes.");
        return;
    }
    
    terminal_write(buf, out_len);
}
