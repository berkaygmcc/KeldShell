#include "conversion.h"

static int count_digits(long n){

    int digits = 0;

    if(n == 0){
        digits = 1;
    }

    if(n < 0){
        digits++;
    }

    while(n != 0){
        digits++;
        n /= 10;
    }

    return digits;
}

char *itoa(int n){

    long num = n;

    int digits = count_digits(num);

    char *c = malloc(digits + 1);

    if(c == NULL){
        return NULL;
    }

    int end_index = 0;

    if(num < 0 ){
        c[0] = '-';
        end_index = 1;
        num = num * (-1);
    }

    if(num == 0){
        c[0] = '0';

        c[1] = '\0';

        return c;
    }

    for(int i = digits - 1; i != end_index - 1; i--){ 

        int num_digit = num % 10;

        c[i] = '0' + num_digit;

        num /= 10;        
    }

    c[digits] = '\0';

    

    return c;
}