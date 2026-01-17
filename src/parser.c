#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

size_t tokenize(const char *input, Token *tokens, size_t max_tokens)
{
    size_t pos = 0, tcount = 0;

    while (input[pos] != '\0')
    {
        while (isspace((unsigned char)input[pos]))
            pos++; 
        if (input[pos] == '\0')
            break;
        if (tcount >= max_tokens - 1)
            break; 

        if (input[pos] == '|')
        {
            tokens[tcount++] = (Token){TOK_PIPE, NULL};
            pos++;
        }
        else if (input[pos] == '&')
        {
            tokens[tcount++] = (Token){TOK_AMP, NULL};
            pos++;
        }
        else if (input[pos] == ';')
        {
            tokens[tcount++] = (Token){TOK_SEMI, NULL};
            pos++;
        }
        else if (input[pos] == '<')
        {
            tokens[tcount++] = (Token){TOK_INPUT, NULL};
            pos++;
        }
        else if (input[pos] == '>')
        {
            if (input[pos + 1] == '>')
            {
                tokens[tcount++] = (Token){TOK_APPEND, NULL};
                pos += 2;
            }
            else
            {
                tokens[tcount++] = (Token){TOK_OUTPUT, NULL};
                pos++;
            }
        }
        else
        {
            // NAME
            size_t start = pos;
            while (input[pos] != '\0' &&
                   !isspace((unsigned char)input[pos]) &&
                   input[pos] != '|' && input[pos] != '&' &&
                   input[pos] != ';' && input[pos] != '<' &&
                   input[pos] != '>')
            {
                pos++;
            }
            size_t len = pos - start;
            char *name = malloc(len + 1);
            if (!name)
                break;
            strncpy(name, &input[start], len);
            name[len] = '\0';
            tokens[tcount++] = (Token){TOK_NAME, name};
        }
    }

    tokens[tcount++] = (Token){TOK_END, NULL};
    return tcount;
}

static size_t pos;
static Token *tok;

static int accept(TokenType type)
{
    if (tok[pos].type == type)
    {
        pos++;
        return 1;
    }
    return 0;
}

static int parse_atomic(void)
{
    if (!accept(TOK_NAME))
        return 0;
    // (NAME | INPUT NAME | OUTPUT NAME | APPEND NAME)*
    while (tok[pos].type == TOK_NAME ||
           tok[pos].type == TOK_INPUT ||
           tok[pos].type == TOK_OUTPUT ||
           tok[pos].type == TOK_APPEND)
    {
        TokenType t = tok[pos].type;
        pos++;
        if (t == TOK_INPUT || t == TOK_OUTPUT || t == TOK_APPEND)
        {
            if (!accept(TOK_NAME))
                return 0;
        }
    }
    return 1;
}

static int parse_cmd_group(void)
{
    if (!parse_atomic())
        return 0;
    while (accept(TOK_PIPE))
    {
        if (!parse_atomic())
            return 0;
    }
    return 1;
}
int parse_shell_cmd(Token *tokens, size_t ntokens)
{
    pos = 0;
    tok = tokens;

    if (!parse_cmd_group())
        return 0;

    for (;;)
    {
        if (tok[pos].type == TOK_SEMI)
        {
            pos++;
            if (!parse_cmd_group())
                return 0;
            continue;
        }
        else if (tok[pos].type == TOK_AMP)
        {
            if (tok[pos + 1].type == TOK_NAME)
            {
                pos++;
                if (!parse_cmd_group())
                    return 0;
                continue;
            }
            else
            {
                break;
            }
        }
        break;
    }

    accept(TOK_AMP);

    return tok[pos].type == TOK_END;
}

