#include <kernel/klogging.h>
#include <kernel/bufferutils.h>
#include <kernel/spinlock.h>
#include <klib/stdio.h>
#include <klib/errno.h>
#include <klib/utils.h>

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

            format++; // Necessary to go over the second % to not include it twice
            continue;
        }

        curr_ch = *format++;
        switch(curr_ch){
            case 'd': {
                int val = va_arg(args, int);
                char num_buf[MAX_INT_DIGITS]; // Enough digits for 64 bit int
                int int_str_len = itoa(val, num_buf);

                for(int i = 0; i < int_str_len; i++){
                    if(!BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, num_buf[i]))
                        return -EOVERFLOW;
                }

                break;
            }
            case 's': {
                char *str = va_arg(args, char*);
                if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, str))
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
            case 'p': {
                void *ptr = va_arg(args, void *);
                if(ptr == NULL){
                   char msg[] = "0x0";
                   if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, msg))
                       return -EOVERFLOW;
                   break;
                }
                uint64_t ptr_val = (uint64_t)ptr;
                char hex_str[MAX_HEX_DIGITS];
                ui64_to_hex_str(ptr_val, hex_str);

                if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, hex_str))
                    return -EOVERFLOW;
                break;
            }
            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                char hex_str[MAX_HEX_DIGITS];
                ui64_to_hex_str(val, hex_str);
                if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, hex_str))
                    return -EOVERFLOW;
                break;
            }
            case 'l': {
                if(*format == 'u'){
                    format++;
                    unsigned long val = va_arg(args, unsigned long);
                    char ul_str[32];
                    ul_to_str(val, ul_str);
                    if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, ul_str))
                        return -EOVERFLOW;
                    break;
                }
                if(*format == 'x'){
                    format++;
                    uint64_t val = va_arg(args, uint64_t);
                    char hex_str[MAX_HEX_DIGITS];
                    ui64_to_hex_str(val, hex_str);
                    if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, hex_str))
                        return -EOVERFLOW;
                    break;
                }
                if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, "%l") ||
                    !BUFFER_SAFE_WRITE_CH(buf, len, KPRINTF_BUF_SIZE, *format++))
                    return -EOVERFLOW;
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                char ul_str[MAX_INT_DIGITS];
                ul_to_str(val, ul_str);
                if(!BUFFER_SAFE_WRITE_STR(buf, len, KPRINTF_BUF_SIZE, ul_str))
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

static DEFINE_SPINLOCK(kprint_lock);

void kprintf(const char* restrict format, ...){
    char buf[KPRINTF_BUF_SIZE];

    va_list args;
    va_start(args, format);

    int out_len = kvsprintf(buf, format, args);

    va_end(args);

    if(out_len < 0){
        int_flags flags;
        spinlock_lock_intsave(&kprint_lock, &flags);
        terminal_writestring("Error: Buffer overflow - data exceeds the allowed buffer size of 1024 chars.");
        spinlock_unlock_intrestore(&kprint_lock, flags);
        
        return;
    }
    int_flags flags;
    spinlock_lock_intsave(&kprint_lock, &flags);
    terminal_write(buf, out_len);
    spinlock_unlock_intrestore(&kprint_lock, flags);
}

void KSUCCESS(const char* restrict format, ...){
    char buf[KPRINTF_BUF_SIZE];
    
    va_list args;
    va_start(args, format);

    int out_len = kvsprintf(buf, format, args);

    va_end(args);

    if(out_len < 0){
        int_flags flags;
        spinlock_lock_intsave(&kprint_lock, &flags);
        terminal_writestring("Error: Buffer overflow - data exceeds the allowed buffer size of 1024 chars.");
        spinlock_unlock_intrestore(&kprint_lock, flags);
        
        return;
    }
    
    int_flags flags;
    spinlock_lock_intsave(&kprint_lock, &flags);
    terminal_writestring("[");
    terminal_setcolor(0x00FF00, 0x000035);
    terminal_writestring("OK");
    terminal_setcolor(0xFFFFFF, 0x000035);
    terminal_writestring("] ");
    terminal_write(buf, out_len);
    spinlock_unlock_intrestore(&kprint_lock, flags);
}

void KWARN(const char* restrict format, ...){
    char buf[KPRINTF_BUF_SIZE];
    
    va_list args;
    va_start(args, format);

    int out_len = kvsprintf(buf, format, args);

    va_end(args);

    if(out_len < 0){
        int_flags flags;
        spinlock_lock_intsave(&kprint_lock, &flags);
        terminal_writestring("Error: Buffer overflow - data exceeds the allowed buffer size of 1024 chars.");
        spinlock_unlock_intrestore(&kprint_lock, flags);
        
        return;
    }
    
    int_flags flags;
    spinlock_lock_intsave(&kprint_lock, &flags);
    terminal_writestring("[");
    terminal_setcolor(0xFFFF00, 0x000035);
    terminal_writestring("WARNING");
    terminal_setcolor(0xFFFFFF, 0x000035);
    terminal_writestring("] ");
    terminal_write(buf, out_len);
    spinlock_unlock_intrestore(&kprint_lock, flags);
}

void KERROR(const char* restrict format, ...){
    char buf[KPRINTF_BUF_SIZE];
    
    va_list args;
    va_start(args, format);

    int out_len = kvsprintf(buf, format, args);

    va_end(args);

    if(out_len < 0){
        int_flags flags;
        spinlock_lock_intsave(&kprint_lock, &flags);
        terminal_writestring("Error: Buffer overflow - data exceeds the allowed buffer size of 1024 chars.");
        spinlock_unlock_intrestore(&kprint_lock, flags);
        
        return;
    }
    
    int_flags flags;
    spinlock_lock_intsave(&kprint_lock, &flags);
    terminal_writestring("[");
    terminal_setcolor(0xFF0000, 0x000035);
    terminal_writestring("ERROR");
    terminal_setcolor(0xFFFFFF, 0x000035);
    terminal_writestring("] ");
    terminal_write(buf, out_len);
    spinlock_unlock_intrestore(&kprint_lock, flags);
}

