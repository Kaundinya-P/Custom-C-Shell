#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

// Token types for the grammar
typedef enum {
    TOK_NAME,
    TOK_PIPE,    
    TOK_AMP,     
    TOK_SEMI,    
    TOK_INPUT,   
    TOK_OUTPUT,  
    TOK_APPEND,  
    TOK_END,
    TOK_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char *text; 
} Token;

size_t tokenize(const char *input, Token *tokens, size_t max_tokens);

int parse_shell_cmd(Token *tokens, size_t ntokens);
int parse_command(const char *input);


#endif
