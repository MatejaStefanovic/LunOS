#include <klib/utils.h>
#include <klib/string.h>

void reverse_str(char *str){
    size_t end = strlen(str)-1;
    size_t start = 0;
    while(start < end){
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;

        start++;
        --end;
    }
}

int ui64_to_hex_str(uint64_t val, char *str){
    const char *hex_digits = "0123456789ABCDEF";
    int index = 0;
    if(val == 0){
        str[index++] = '0';
        str[index] = '\0';
        return index;
    }

    while(val > 0){
        str[index++] = hex_digits[val & 0xF];
        val >>= 4;
    }

    //str[index++] = 'x';
    //str[index++] = '0';

    str[index] = '\0';
    reverse_str(str);

    return index;
}

int ul_to_str(unsigned long value, char *str) {
    int i = 0;

    // Handle zero explicitly
    if (value == 0) {
        str[i++] = '0';
    } else {
        while (value > 0) {
            str[i++] = '0' + (value % 10);
            value /= 10;
        }
    }
    str[i] = '\0';
    reverse_str(str);
    return i;
}
