#include <klib/stdio.h>
#include <klib/utils.h>
// Integer to ASCII that returns the length of the string
int itoa(int i, char *str){
    int index = 0;
    int sign = 1; // positive number
    if(i < 0){
        sign = 0; // negative number
        i = -i;    
    }

    do {
        str[index++] = i % 10 + '0';
    } while((i /= 10) > 0);

    if(!sign)
        str[index++] = '-';
    
    str[index] = '\0';
    reverse_str(str);
   
    return index; // Index starts at 0 so total length is higher by 1
}
