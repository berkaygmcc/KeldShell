#include "lexer.h"
#include "cleanup.h"
#include "utils.h"
#include <ctype.h>

typedef enum{
    STATE_SPACE,
    STATE_WORD,
    STATE_OPERATOR,
    STATE_SINGLE_QUOTE,
    STATE_DOUBLE_QUOTE,
    STATE_VARIABLE
} lexer_state;

typedef enum{
    LEXER_ERR_EMPTY,
    LEXER_ERR_MALLOC,
    LEXER_ERR_REALLOC,
    LEXER_ERR_STRDUP,
    LEXER_ERR_TOO_MANY_TOKENS,
    LEXER_ERR_TOO_MANY_SEGMENTS,
    LEXER_ERR_ADD_TOKEN,
    LEXER_ERR_UNMATCHED_QUOTE,
    LEXER_ERR_UNMATCHED_BRACE,
    LEXER_ERR_SYNTAX
} t_lexer_error;

static const char *operator_chars = "<>|&;()";

int escaped_segment = 0;

static int check_capacity(int *capacity, int s_buffer_index, char **segment_buffer){

    if(s_buffer_index >= *capacity - 1){


        int new_capacity = (*capacity) * 2;

        char *tmp = realloc(*segment_buffer, new_capacity);

        if(tmp == NULL){
            return -1;
        }

        *segment_buffer = tmp;
        *capacity = new_capacity;

    }

    return 0;

}

static int lexer_error(t_token *token_list, char *segment_buffer, t_lexer_error error){

    if(error == LEXER_ERR_EMPTY){
        fprintf(stderr, "shell: empty input\n");
    }
    else if(error == LEXER_ERR_MALLOC){
        fprintf(stderr, "shell: memory allocation failed\n");
    }
    else if(error == LEXER_ERR_REALLOC){
        fprintf(stderr, "shell: failed to reallocate memory\n");
    }
    else if(error == LEXER_ERR_STRDUP){
        fprintf(stderr, "shell: failed to duplicate token\n");
    }
    else if(error == LEXER_ERR_TOO_MANY_TOKENS){
        fprintf(stderr, "shell: too many tokens\n");
    }
    else if(error == LEXER_ERR_TOO_MANY_SEGMENTS){
        fprintf(stderr, "shell: too many segments\n");
    }
    else if(error == LEXER_ERR_ADD_TOKEN){
        fprintf(stderr, "shell: token add error\n");
    }
    else if(error == LEXER_ERR_UNMATCHED_QUOTE){
        fprintf(stderr, "shell: unmatched quote\n");
    }
    else if(error == LEXER_ERR_UNMATCHED_BRACE){
        fprintf(stderr, "shell: unmatched brace\n");
    }
    else if(error == LEXER_ERR_SYNTAX){
        fprintf(stderr, "shell: unexpected token\n");
    }


    free(segment_buffer);
    free_token_list(token_list);

    return -1;
}

static int is_operator(char c){

    for(int i = 0; operator_chars[i] != '\0'; i++){
        if(c == operator_chars[i]){
            return 0;
        }
    }
    return -1;

}

static int is_quote(char c){
    if(c == '"' || c == '\''){
        return 0;
    }
    return -1;
}

static int is_dollar(char c){
    if(c == '$'){
        return 0;
    }
    return -1;
}

static int is_escaped(char c){
    if(c == '$' || c == '`' || c == '"' || c == '\\'){
        return 0;
    }
    return -1;
}

static int is_empty_input(char *input){

    int is_space = 1;

    for(int i = 0; input[i] != '\0' && input[i] != '\n'; i++){
        if(isspace((unsigned char)input[i]) == 0){
            is_space = 0;
            break;
        }
    }

    if(is_space){
        return -1;
    }
    
    return 0;

}

static lexer_state get_quote_type(char c){
    if(c == '"'){
        return STATE_DOUBLE_QUOTE;
    }
    if(c == '\''){
        return STATE_SINGLE_QUOTE;
    }

    return STATE_SPACE;
}

static t_type get_token_type(t_token *token){


    if (!token || token->segment_count == 0 || !token->segments[0].value) {
        return TOKEN_WORD; 
    }

    if(strcmp(token->segments[0].value, "|") == 0){ 
        return TOKEN_PIPE;
    }
    else if(strcmp(token->segments[0].value, ";") == 0){ 
        return TOKEN_SEMICOLON;
    }
    else if(strcmp(token->segments[0].value, "&") == 0){ 
        return TOKEN_BACKGROUND;
    }
    else if(strcmp(token->segments[0].value, "&&") == 0){ 
        return TOKEN_AND;
    }
    else if(strcmp(token->segments[0].value, "||") == 0){ 
        return TOKEN_OR;
    }
    else if(strcmp(token->segments[0].value, "(") == 0){ 
        return TOKEN_LPAREN;
    }
    else if(strcmp(token->segments[0].value, "((") == 0){ 
        return TOKEN_DLPAREN;
    }
    else if(strcmp(token->segments[0].value, ")") == 0){ 
        return TOKEN_RPAREN;
    }
    else if(strcmp(token->segments[0].value, "))") == 0){ 
        return TOKEN_DRPAREN;
    }
    else if(strcmp(token->segments[0].value, "<") == 0){ 
        return TOKEN_REDIR_IN;
    }
    else if(strcmp(token->segments[0].value, ">") == 0){ 
        return TOKEN_REDIR_OUT;
    }
    else if(strcmp(token->segments[0].value, "<<") == 0){ 
        return TOKEN_REDIR_HEREDOC;
    }
    else if(strcmp(token->segments[0].value, ">>") == 0){ 
        return TOKEN_REDIR_APPEND;
    }
    else if(strcmp(token->segments[0].value, "2>") == 0){ 
        return TOKEN_REDIR_ERR_OUT;
    }
    else if(strcmp(token->segments[0].value, "2>>") == 0){ 
        return TOKEN_REDIR_ERR_APPEND;
    }

    return TOKEN_WORD;
}

static int add_segment(t_token *token_list, char *segment_buffer, int t_list_index, int *s_index, int *s_buffer_index, t_quote quote, int escaped){


    segment_buffer[*s_buffer_index] = '\0';

    if(*s_index >= MAX_SEGMENTS){
        return lexer_error(token_list, segment_buffer, LEXER_ERR_TOO_MANY_SEGMENTS);
    }

    token_list[t_list_index].segments[*s_index].value = strdup(segment_buffer);
    token_list[t_list_index].segments[*s_index].quote = quote;
    token_list[t_list_index].segments[*s_index].escaped = escaped;


    if(token_list[t_list_index].segments[*s_index].value == NULL){
        return lexer_error(token_list, segment_buffer, LEXER_ERR_STRDUP);
    }

    token_list[t_list_index].segment_count++;
    (*s_index)++;
    *s_buffer_index = 0;
    

    return 0;
}

static int add_token(t_token *token_list, char *segment_buffer, int *t_list_index, int *s_index, int *s_buffer_index, t_quote quote){

    if(*t_list_index >= MAX_ARGS){
        return lexer_error(token_list, segment_buffer, LEXER_ERR_TOO_MANY_TOKENS);
    }


    
    if(*s_buffer_index > 0){
        if(add_segment(token_list, segment_buffer, *t_list_index, s_index, s_buffer_index, quote, escaped_segment) == -1){
            return -1;
        }   

        escaped_segment = 0;
    }   

    token_list[*t_list_index].type = get_token_type(&token_list[*t_list_index]);

    (*t_list_index)++;

    if (*t_list_index < MAX_ARGS){
        token_list[*t_list_index].segment_count = 0;
    }

    *s_index = 0;

    return 0;

}


int tokenize_input(char *input, t_token *token_list){

    int t_list_index = 0;

    int s_index = 0;

    int capacity = 16;

    char *segment_buffer = malloc(capacity);

    int s_buffer_index = 0;
    
    if(segment_buffer == NULL){ 
        return lexer_error(token_list, segment_buffer, LEXER_ERR_MALLOC);
    }

    if(is_empty_input(input) == -1){
        token_list[0].type = TOKEN_END;
        token_list[0].segment_count = 0;
        return lexer_error(token_list, segment_buffer, LEXER_ERR_EMPTY);
    }

    lexer_state state = STATE_SPACE;

    t_quote current_quote = QUOTE_NONE;

    for(int i = 0; input[i] != '\0'; i++){

        //printf("CHAR=%c STATE=%d\n", input[i], state);

        if(isspace((unsigned char)input[i]) == 0){
            

            if(check_capacity(&capacity, s_buffer_index, &segment_buffer) == -1){
                return lexer_error(token_list, segment_buffer, LEXER_ERR_REALLOC);
            }

            if(is_quote(input[i]) == 0){
                if(state == get_quote_type(input[i])){
                    

                    if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                        return -1;
                    }

                    escaped_segment = 0;

                    current_quote = QUOTE_NONE;

                    state = STATE_WORD;        
                }
                else if(state == STATE_SINGLE_QUOTE || state == STATE_DOUBLE_QUOTE){
                    segment_buffer[s_buffer_index] = input[i];
                    s_buffer_index++;
                }
                else{
                    
                    if(s_buffer_index > 0){
                        if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                            return -1;
                        }
                        escaped_segment = 0;
                    }


                    state = get_quote_type(input[i]);

                    if(state == STATE_SINGLE_QUOTE){
                        current_quote = QUOTE_SINGLE;
                    }
                    else if(state == STATE_DOUBLE_QUOTE){
                        current_quote = QUOTE_DOUBLE;
                    }
                }
            }          
            else if(is_operator(input[i]) == 0){

                if(state == STATE_WORD){
                    if(input[i] == '>' && s_buffer_index == 1 && segment_buffer[0] == '2' && current_quote == QUOTE_NONE && escaped_segment == 0){

                        state = STATE_OPERATOR;

                        segment_buffer[s_buffer_index] = input[i];
                        s_buffer_index++;

                        continue;
                    }

                    if(add_token(token_list, segment_buffer, &t_list_index, &s_index, &s_buffer_index, current_quote) == -1){
                        return -1;
                    }
                    current_quote = QUOTE_NONE;
                }
                else if(state == STATE_OPERATOR){

                    if(input[i] == '>' && s_buffer_index == 2 && segment_buffer[0] == '2' && segment_buffer[1] == '>'){

                        segment_buffer[s_buffer_index] = input[i];
                        s_buffer_index++;

                        continue;
                    }


                    if(segment_buffer[0] != input[i]){ 
                        if(add_token(token_list, segment_buffer, &t_list_index, &s_index, &s_buffer_index, current_quote) == -1){
                            return -1;
                        }
                    }
                    else if(s_buffer_index >= 2){
                        return lexer_error(token_list, segment_buffer, LEXER_ERR_SYNTAX);
                    }
                    current_quote = QUOTE_NONE;
                }
                
                if(state != STATE_SINGLE_QUOTE && state != STATE_DOUBLE_QUOTE){
                    state = STATE_OPERATOR;
                }

                segment_buffer[s_buffer_index] = input[i];
                s_buffer_index++;
            }
            else if(input[i] == '\\' && state != STATE_SINGLE_QUOTE){

                if(state == STATE_DOUBLE_QUOTE){

                    if(is_escaped(input[i + 1]) == 0){

                        if(s_buffer_index > 0){
                            if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                                return -1;
                            } 
                            escaped_segment = 0; 
                        }

                        escaped_segment = 1;
                        
                        i++;

                        if(input[i] != '\0'){
                            segment_buffer[s_buffer_index] = input[i];
                            s_buffer_index++;
                        }
                    }
                    else{
                        segment_buffer[s_buffer_index] = input[i];
                        s_buffer_index++;
                    }
                }
                else{


                    if(state == STATE_SPACE){
                        state = STATE_WORD;
                    }


                    if(s_buffer_index > 0){
                        if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                            return -1;
                        } 
                        escaped_segment = 0; 
                    }

                    escaped_segment = 1;

                    i++;

                    if(input[i] != '\0'){
                        segment_buffer[s_buffer_index] = input[i];
                        s_buffer_index++;
                    }
                }
            }
            else if(is_dollar(input[i]) == 0 && state != STATE_SINGLE_QUOTE){

                if(state == STATE_SPACE){
                    state = STATE_WORD;
                }

                if(s_buffer_index > 0){
                    if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                        return -1;
                    }   
                    escaped_segment = 0; 
                }       

                segment_buffer[s_buffer_index] = input[i];
                s_buffer_index++;
                
                i++;

                if(input[i] == '?' || input[i] == '$'){
                                    
                    segment_buffer[s_buffer_index] = input[i];
                    s_buffer_index++;
                    i++;

                    if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                        return -1;
                    }   
                    escaped_segment = 0;

                }
                else if(input[i] == '{'){

                    i++;

                    if(input[i] == '}'){
                        return lexer_error(token_list, segment_buffer, LEXER_ERR_SYNTAX); 
                    }
                    else{
                        while(is_variable(input[i]) != -1){
                        
                            if(check_capacity(&capacity, s_buffer_index, &segment_buffer) == -1){
                                return lexer_error(token_list, segment_buffer, LEXER_ERR_REALLOC);
                            }

                            segment_buffer[s_buffer_index] = input[i];
                            s_buffer_index++;
                            i++;
                        }

                        if(input[i] != '}'){
                            return lexer_error(token_list, segment_buffer, LEXER_ERR_UNMATCHED_BRACE); 
                        }

                        if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                            return -1;
                        } 
                        escaped_segment = 0;

                    }

                    i++;

                }
                else{
                    while(is_variable(input[i]) != -1){

                        //printf("CHAR=%c STATE=%d\n", input[i], state);

                        if(check_capacity(&capacity, s_buffer_index, &segment_buffer) == -1){
                            return lexer_error(token_list, segment_buffer, LEXER_ERR_REALLOC);
                        }

                    
                        segment_buffer[s_buffer_index] = input[i];
                        s_buffer_index++;

                        i++;
                    }

                    //printf("variable finished at '%c'\n", input[i]);

                    
                    if(add_segment(token_list, segment_buffer, t_list_index, &s_index, &s_buffer_index, current_quote, escaped_segment) == -1){
                        return -1;
                    }        
                    escaped_segment = 0;            
                }

                i--;                
                
            }
            else{

                if(state == STATE_OPERATOR){
                    if(add_token(token_list, segment_buffer, &t_list_index, &s_index, &s_buffer_index, current_quote) == -1){
                        return -1;
                    }    
                    current_quote = QUOTE_NONE;           
                }

                if(state != STATE_SINGLE_QUOTE && state != STATE_DOUBLE_QUOTE){
                    state = STATE_WORD;
                }


                segment_buffer[s_buffer_index] = input[i];
                s_buffer_index++;

            }
        }
        else{
            if(state == STATE_WORD || state == STATE_OPERATOR){     

                if(add_token(token_list, segment_buffer, &t_list_index, &s_index, &s_buffer_index, current_quote) == -1){
                    return -1;
                }

                current_quote = QUOTE_NONE;
                state = STATE_SPACE;
            }
            else if(state == STATE_SINGLE_QUOTE || state == STATE_DOUBLE_QUOTE){
                segment_buffer[s_buffer_index] = input[i];
                s_buffer_index++;
            }
        }
    }

    if(state == STATE_SINGLE_QUOTE || state == STATE_DOUBLE_QUOTE){
        return lexer_error(token_list, segment_buffer, LEXER_ERR_UNMATCHED_QUOTE); 
    }

    if(state == STATE_WORD || state == STATE_OPERATOR){

        if(add_token(token_list, segment_buffer, &t_list_index, &s_index, &s_buffer_index, current_quote) == -1){
            return -1;
        }

        current_quote = QUOTE_NONE;

    }

    token_list[t_list_index].type = TOKEN_END;



    free(segment_buffer);


    return 0;

}

