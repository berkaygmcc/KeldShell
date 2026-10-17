#include "heredoc_expand.h"
#include "conversion.h"
#include "utils.h"

typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
} t_builder;

static int append_char(t_builder *builder, char c){

    if(builder->length + 1 >= builder->capacity){
        char *tmp = realloc(builder->buffer, builder->capacity * 2);

        if(tmp == NULL){
            return -1;
        }

        builder->buffer = tmp;
        builder->capacity *= 2;
    }

    builder->buffer[builder->length] = c;

    builder->length++;

    builder->buffer[builder->length] = '\0';

    return 0;
}

static int append_str(t_builder *builder, char *str){
    for(int i = 0; str[i] != '\0'; i++){
        if(append_char(builder, str[i]) == -1){
            return -1;
        }
    }

    return 0;
}


static int expand_special_variable(char *line, int *index, t_builder *builder, t_shell *shell){

    if(line[*index] == '$'){
        char *value = itoa(getpid());
        if(append_str(builder, value) == -1){
            free(value);
            return -1;
        }
        free(value);
    }
    else if(line[*index] == '?'){
        char *value = itoa(shell->exit_status);
        if(append_str(builder, value) == -1){
            free(value);
            return -1;
        }
        free(value);
    }

    return 0;
}

static char *expand_variable(char *line, int *index){

    char *value = malloc(MAX_VAR_LEN);

    if(value == NULL){
        return NULL;
    }

    int i = 0;

    while(is_variable(line[*index]) != -1 && i < MAX_VAR_LEN - 1){
        value[i] = line[*index];
        i++;

        (*index)++;
    }

    value[i] = '\0';

    return value;

}

static int copy_invalid_brace(char *line, int *index, int start_index, t_builder *builder){

    int brace = 0;

    while(line[*index] != '\0'){

        if(line[*index] == '{'){
            brace++;
        }
        else if(line[*index] == '}'){
            brace --;

            if(brace == 0){
                (*index)++;
                break;
            }
        }

        (*index)++;

    }

    while(start_index < *index){

        if(append_char(builder, line[start_index]) == -1){
            return -1;
        }
        start_index++;        
    }

    return 0;
}

static int expand_dollar(char *line, int *index, t_builder *builder, t_shell *shell){

    int start_index = *index;

    (*index)++;

    if(line[*index] == '{'){

        (*index)++;

        if(line[*index] == '}'){
            while(start_index <= *index){
                if(append_char(builder, line[start_index]) == -1){
                    return -1;
                }
                start_index++;
            }
            return 0;
        } 

        char *value = expand_variable(line, index);

        if(line[*index] == '}'){
            char *env = getenv(value);

            if(env != NULL){
                if(append_str(builder, env) == -1){
                    free(value);
                    return -1;
                }
            }

            (*index)++;

        }
        else{
            if(copy_invalid_brace(line, index, start_index, builder) == -1){
                free(value);
                return -1;
            }
        }

        free(value);
    }
    else if(line[*index] == '$' || line[*index] == '?'){
        if(expand_special_variable(line, index, builder, shell) == -1){
            return -1;
        }
        (*index)++;
    }
    else{
        char *value = expand_variable(line, index);
        
        char *env = getenv(value);

        if(env != NULL){
            if(append_str(builder, env) == -1){
                free(value);
                return -1;
            }
        }

        free(value);
        
    }

    return 0;
}

char *expand_heredoc_line(char *line, t_shell *shell){

    t_builder builder;

    builder.buffer = malloc(64);
    if(builder.buffer == NULL){
        return NULL;
    }
    builder.buffer[0] = '\0';
    builder.length = 0;
    builder.capacity = 64;

    int i = 0;

    int escaped = 0;

    while(line[i] != '\0'){

        if(line[i] == '\\'){
            escaped = 1;
            i++;
            continue;
        }


        if(line[i] == '$' && escaped != 1){

            if(line[i + 1] == '\0' || line[i + 1] == ' ' || (line[i + 1] >= '0' && line[i + 1] <= '9')){
                if(append_char(&builder, '$') == -1){
                    free(builder.buffer);
                    return NULL;
                }
                i++;
                continue;
            }

            if(expand_dollar(line, &i, &builder, shell) == -1){
                free(builder.buffer);
                return NULL;
            }
            continue;
        }
        else{
            if(append_char(&builder, line[i]) == -1){
                free(builder.buffer);
                return NULL;
            }
            escaped = 0;
        }

        i++;
    }

    return builder.buffer;
}