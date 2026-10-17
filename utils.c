#include "utils.h"

int copy_token(t_token *dst, t_token *src){

    dst->type = src->type;
    dst->segment_count = src->segment_count;

    for(int i = 0; i < dst->segment_count; i++){

        if (dst->segments[i].value != NULL) {
            free(dst->segments[i].value);
            dst->segments[i].value = NULL;
        }

        dst->segments[i].value = strdup(src->segments[i].value);
        
        if(dst->segments[i].value == NULL){
            return -1;
        }

        dst->segments[i].escaped = src->segments[i].escaped;

        dst->segments[i].quote = src->segments[i].quote;
    }

    return 0;

}

void move_token(t_token *dst, t_token *src){

    *dst = *src;

    memset(src, 0, sizeof(t_token));

}

int is_variable_start(char c){

    if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_'){
        return 0;
    }

    return -1;

}

int is_variable(char c){
    if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_'){
        return 0;
    }
    return -1;
}