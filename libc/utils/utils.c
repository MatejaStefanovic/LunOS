#include <utils.h>

void reverse_str(char *str){
    size_t end = strlen(str)-1;
    size_t start = 0;
    while(start < end){
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;

        ++start;
        --end;
    }
}

