#ifndef PARSER_H
#define PARSER_H

#include "../Lexer/Lexer.h"
#include <stdbool.h>

typedef struct {
    FILE* symbol_table_file;
    Token current_token;
    Token next_token;
    Token previous_token; // For lookbehind
    bool has_next_token;
    
    bool had_error;
    bool panic_mode;
    int indent_level;
    FILE* output_file;
} Parser;

Parser* parser_create(const char* symbol_table_path);
void parser_parse(Parser* parser);

#endif // PARSER_H
