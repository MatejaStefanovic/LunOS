#include <kernel/klogging.h>
#include <kernel/tty.h>
#include <string.h>
#include <stdio.h>

int kvsprintf(char *buf, const char* restrict format, va_list args){
    int len = 0;
    while(*format){
        char curr_ch = *format++;
        
        // Not format specified -> add to buffer
        if(curr_ch != '%'){
            buf[len++] = curr_ch;
            continue;
        }
        // Handle %% case
        if(*format == '%'){
            buf[len++] = curr_ch;
            ++format; // Necessary to go over the second % to not include it twice
            continue;
        }
        switch(*format++){
            case 'd': {
                int val = va_arg(args, int);
                char num_buf[12]; // Enough digits for 32 bit int
                int int_str_len = itoa(val, num_buf);

                for(int i = 0; i < int_str_len; ++i){
                    buf[len++] = num_buf[i];
                }
                break;
            }
            case 's': {
                char *str = va_arg(args, char*);
                while(*str){
                    buf[len++] = *str++;
                }
                break;
            }
            case 'c': {
                /* Smaller types promote to int when there is an unknown
                 * amount of variables therefore it is needed to look for int */
                char ch = va_arg(args, int); 
                buf[len++] = (char) ch;
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
        terminal_write("Err", 3);
        return;
    }
    
    terminal_write(buf, out_len);
}
