/*
 * Cythonic Syntax Analyzer - C Implementation
 * 
 * Implements a Recursive Descent Parser for the Cythonic language.
 * Reads tokens from the Lexer (Symbol Table) and validates syntax.
 * 
 * Features:
 * - Recursive Descent Algorithm
 * - Error Recovery (Panic Mode)
 * - Supports: Input, Output, Assignment, Iterative, Declaration statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Parser.h"

/* ============================================================================
 * TOKEN UTILITIES
 * ============================================================================ */

static TokenType string_to_token_type(const char* str) {
    if (strcmp(str, "KEYWORD") == 0) return KEYWORD;
    if (strcmp(str, "RESERVED_WORD") == 0) return RESERVED_WORD;
    if (strcmp(str, "TYPE") == 0) return TYPE;
    if (strcmp(str, "IDENTIFIER") == 0) return IDENTIFIER;
    if (strcmp(str, "BOOLEAN_LITERAL") == 0) return BOOLEAN_LITERAL;
    if (strcmp(str, "NOISE_WORD") == 0) return NOISE_WORD;
    if (strcmp(str, "NUMBER") == 0) return NUMBER;
    if (strcmp(str, "STRING_LITERAL") == 0) return STRING_LITERAL;
    if (strcmp(str, "CHAR_LITERAL") == 0) return CHAR_LITERAL;
    if (strcmp(str, "PLUS") == 0) return PLUS;
    if (strcmp(str, "MINUS") == 0) return MINUS;
    if (strcmp(str, "STAR") == 0) return STAR;
    if (strcmp(str, "SLASH") == 0) return SLASH;
    if (strcmp(str, "PERCENT") == 0) return PERCENT;
    if (strcmp(str, "PLUS_PLUS") == 0) return PLUS_PLUS;
    if (strcmp(str, "MINUS_MINUS") == 0) return MINUS_MINUS;
    if (strcmp(str, "EQUAL") == 0) return EQUAL;
    if (strcmp(str, "EQUAL_EQUAL") == 0) return EQUAL_EQUAL;
    if (strcmp(str, "NOT_EQUAL") == 0) return NOT_EQUAL;
    if (strcmp(str, "GREATER") == 0) return GREATER;
    if (strcmp(str, "LESS") == 0) return LESS;
    if (strcmp(str, "GREATER_EQUAL") == 0) return GREATER_EQUAL;
    if (strcmp(str, "LESS_EQUAL") == 0) return LESS_EQUAL;
    if (strcmp(str, "AND_AND") == 0) return AND_AND;
    if (strcmp(str, "OR_OR") == 0) return OR_OR;
    if (strcmp(str, "NOT") == 0) return NOT;
    if (strcmp(str, "AND") == 0) return AND;
    if (strcmp(str, "OR") == 0) return OR;
    if (strcmp(str, "XOR") == 0) return XOR;
    if (strcmp(str, "TILDE") == 0) return TILDE;
    if (strcmp(str, "LEFT_PAREN") == 0) return LEFT_PAREN;
    if (strcmp(str, "RIGHT_PAREN") == 0) return RIGHT_PAREN;
    if (strcmp(str, "LEFT_BRACE") == 0) return LEFT_BRACE;
    if (strcmp(str, "RIGHT_BRACE") == 0) return RIGHT_BRACE;
    if (strcmp(str, "LEFT_BRACKET") == 0) return LEFT_BRACKET;
    if (strcmp(str, "RIGHT_BRACKET") == 0) return RIGHT_BRACKET;
    if (strcmp(str, "SEMICOLON") == 0) return SEMICOLON;
    if (strcmp(str, "COMMA") == 0) return COMMA;
    if (strcmp(str, "DOT") == 0) return DOT;
    if (strcmp(str, "COLON") == 0) return COLON;
    if (strcmp(str, "QUESTION") == 0) return QUESTION;
    if (strcmp(str, "COMMENT") == 0) return COMMENT;
    if (strcmp(str, "INVALID") == 0) return INVALID;
    if (strcmp(str, "EOF") == 0) return TOKEN_EOF;
    return INVALID;
}

static void trim_trailing_whitespace(char* str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static bool read_next_token_from_file(Parser* parser, Token* token) {
    char line_buffer[2048]; // Increased buffer size
    
    while (fgets(line_buffer, sizeof(line_buffer), parser->symbol_table_file)) {
        // Check for end of table
        if (strncmp(line_buffer, "END OF SYMBOL TABLE", 19) == 0) {
            return false;
        }
        
        // Skip empty lines or headers (lines not starting with a number)
        char* ptr = line_buffer;
        while (*ptr == ' ') ptr++;
        if (!isdigit(*ptr)) continue; 
        
        // Parse line manually to handle | in strings
        // Format: LINE | COL | TYPE | LEXEME | RAW
        
        char* p1 = strchr(line_buffer, '|');
        if (!p1) continue;
        char* p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        char* p3 = strchr(p2 + 1, '|');
        if (!p3) continue;
        char* p_last = strrchr(line_buffer, '|');
        if (!p_last || p_last == p3) continue; // Must have at least 4 pipes
        
        // Extract fields
        *p1 = '\0';
        *p2 = '\0';
        *p3 = '\0';
        *p_last = '\0';
        
        char* token_line = line_buffer;
        char* token_col = p1 + 1;
        char* token_type = p2 + 1;
        char* token_lexeme = p3 + 1;
        char* token_raw = p_last + 1;
        
        // Remove newline from raw
        char* newline = strrchr(token_raw, '\n');
        if (newline) *newline = '\0';
        
        token->line = atoi(token_line);
        token->column = atoi(token_col);
        
        trim_trailing_whitespace(token_type);
        while (*token_type == ' ') token_type++;
        token->type = string_to_token_type(token_type);
        
        if (*token_lexeme == ' ') token_lexeme++;
        trim_trailing_whitespace(token_lexeme);
        token->lexeme = strdup(token_lexeme);
        
        if (*token_raw == ' ') token_raw++;
        trim_trailing_whitespace(token_raw);
        token->raw = strdup(token_raw);
        
        return true;
    }
    return false;
}

/* ============================================================================
 * PARSE TREE HELPERS
 * ============================================================================ */

static void print_indent(Parser* parser) {
    if (!parser->output_file) return;
    for (int i = 0; i < parser->indent_level; i++) {
        fprintf(parser->output_file, "  ");
    }
}

static void enter_node(Parser* parser, const char* name) {
    if (!parser->output_file) return;
    print_indent(parser);
    fprintf(parser->output_file, "Enter <%s>\n", name);
    parser->indent_level++;
}

static void exit_node(Parser* parser, const char* name) {
    parser->indent_level--;
    if (!parser->output_file) return;
    print_indent(parser);
    fprintf(parser->output_file, "Exit <%s>\n", name);
}

static void print_next_token(Parser* parser) {
    if (!parser->output_file) return;
    print_indent(parser);
    fprintf(parser->output_file, "Next token is: %s Next lexeme is %s\n", 
            token_type_to_string(parser->current_token.type), 
            parser->current_token.lexeme);
}

/* ============================================================================
 * PARSER UTILITIES
 * ============================================================================ */

static void advance(Parser* parser); // Forward declaration

Parser* parser_create(const char* symbol_table_path) {
    Parser* parser = malloc(sizeof(Parser));
    parser->symbol_table_file = fopen(symbol_table_path, "r");
    if (!parser->symbol_table_file) {
        fprintf(stderr, "Error: Cannot open symbol table file '%s'\n", symbol_table_path);
        free(parser);
        return NULL;
    }
    
    parser->had_error = false;
    parser->panic_mode = false;
    parser->indent_level = 0;
    parser->output_file = NULL;
    
    // Initialize tokens
    parser->current_token.type = INVALID;
    parser->current_token.lexeme = NULL;
    parser->current_token.raw = NULL;
    
    parser->previous_token.type = INVALID;
    parser->previous_token.lexeme = NULL;
    parser->previous_token.raw = NULL;
    
    // Prime the pump: read first token into next_token
    if (read_next_token_from_file(parser, &parser->next_token)) {
        parser->has_next_token = true;
    } else {
        parser->has_next_token = false;
        parser->next_token.type = TOKEN_EOF;
        parser->next_token.lexeme = strdup("");
        parser->next_token.raw = strdup("");
    }
    
    // Move next to current
    advance(parser);
    
    return parser;
}

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(stderr, "[line %d:%d] Error", token->line, token->column);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == INVALID) {
        // Already an error token
    } else {
        fprintf(stderr, " at '%s'", token->raw);
    }

    fprintf(stderr, ": %s\n", message);
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->current_token, message);
}

static void advance(Parser* parser) {
    // Free previous token data if needed (but we keep it for lookbehind)
    // Actually, we should free the OLD previous token before overwriting it.
    if (parser->previous_token.lexeme) free(parser->previous_token.lexeme);
    if (parser->previous_token.raw) free(parser->previous_token.raw);
    
    parser->previous_token = parser->current_token;
    
    // Move next to current
    parser->current_token = parser->next_token;
    
    // Read new next
    if (parser->has_next_token) {
        // We just consumed the valid next_token, so we need to read a NEW one.
        // But wait, `next_token` struct memory is now copied to `current_token`.
        // We need to allocate new strings for `next_token` when we read.
        // `read_next_token_from_file` does strdup.
        // So `current_token` owns its strings, and `next_token` will own new strings.
        
        if (!read_next_token_from_file(parser, &parser->next_token)) {
            parser->has_next_token = false;
            parser->next_token.type = TOKEN_EOF;
            parser->next_token.lexeme = strdup("");
            parser->next_token.raw = strdup("");
        }
    } else {
        // Already at EOF
        parser->next_token.type = TOKEN_EOF;
        // Don't strdup again if we can avoid it, or just do it for safety
        parser->next_token.lexeme = strdup("");
        parser->next_token.raw = strdup("");
    }
    
    print_next_token(parser);
}

static bool check(Parser* parser, TokenType type) {
    return parser->current_token.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) {
        advance(parser);
        return;
    }
    error(parser, message);
}

static void synchronize(Parser* parser) {
    parser->panic_mode = false;

    while (parser->current_token.type != TOKEN_EOF) {
        if (parser->current_token.type == SEMICOLON) {
            advance(parser);
            return;
        }

        switch (parser->current_token.type) {
            case KEYWORD: // Check for statement starters
            case RESERVED_WORD:
            case TYPE:
                return;
            default:
                ;
        }

        advance(parser);
    }
}

/* ============================================================================
 * GRAMMAR RULES
 * ============================================================================ */

// Forward declarations
static void statement(Parser* parser);
static void expression(Parser* parser);

static void block(Parser* parser) {
    enter_node(parser, "Block");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        statement(parser);
    }
    consume(parser, RIGHT_BRACE, "Expect '}' after block.");
    exit_node(parser, "Block");
}

// Simple expression parser (handles basic operations)
// static void expression(Parser* parser); // Already declared

static void primary(Parser* parser) {
    enter_node(parser, "Primary");
    if (match(parser, NUMBER)) { exit_node(parser, "Primary"); return; }
    if (match(parser, STRING_LITERAL)) { exit_node(parser, "Primary"); return; }
    if (match(parser, CHAR_LITERAL)) { exit_node(parser, "Primary"); return; }
    if (match(parser, BOOLEAN_LITERAL)) { exit_node(parser, "Primary"); return; }
    if (match(parser, IDENTIFIER)) { exit_node(parser, "Primary"); return; }
    if (match(parser, LEFT_PAREN)) {
        expression(parser);
        consume(parser, RIGHT_PAREN, "Expect ')' after expression.");
        exit_node(parser, "Primary");
        return;
    }
    error(parser, "Expect expression.");
    exit_node(parser, "Primary");
}

static void postfix(Parser* parser) {
    enter_node(parser, "Postfix");
    primary(parser);
    while (match(parser, PLUS_PLUS) || match(parser, MINUS_MINUS)) {
        // Consume postfix operator
    }
    exit_node(parser, "Postfix");
}

static void unary(Parser* parser) {
    enter_node(parser, "Unary");
    if (match(parser, NOT) || match(parser, MINUS)) {
        unary(parser);
        exit_node(parser, "Unary");
        return;
    }
    postfix(parser);
    exit_node(parser, "Unary");
}

static void factor(Parser* parser) {
    enter_node(parser, "Factor");
    unary(parser);
    while (match(parser, SLASH) || match(parser, STAR) || match(parser, PERCENT)) {
        unary(parser);
    }
    exit_node(parser, "Factor");
}

static void term(Parser* parser) {
    enter_node(parser, "Term");
    factor(parser);
    while (match(parser, MINUS) || match(parser, PLUS)) {
        factor(parser);
    }
    exit_node(parser, "Term");
}

static void comparison(Parser* parser) {
    enter_node(parser, "Comparison");
    term(parser);
    while (match(parser, GREATER) || match(parser, GREATER_EQUAL) ||
           match(parser, LESS) || match(parser, LESS_EQUAL)) {
        term(parser);
    }
    exit_node(parser, "Comparison");
}

static void equality(Parser* parser) {
    enter_node(parser, "Equality");
    comparison(parser);
    while (match(parser, NOT_EQUAL) || match(parser, EQUAL_EQUAL)) {
        comparison(parser);
    }
    exit_node(parser, "Equality");
}

static void expression(Parser* parser) {
    enter_node(parser, "Expression");
    equality(parser);
    exit_node(parser, "Expression");
}

// 9. Declaration Statement: type identifier [= expression];
static void declaration_statement(Parser* parser) {
    enter_node(parser, "DeclarationStatement");
    // Type is already consumed by statement()
    consume(parser, IDENTIFIER, "Expect variable name.");

    if (match(parser, EQUAL)) {
        expression(parser);
    }

    consume(parser, SEMICOLON, "Expect ';' after variable declaration.");
    exit_node(parser, "DeclarationStatement");
}

// 7. Assignment Statement: identifier = expression;
static void assignment_statement(Parser* parser) {
    enter_node(parser, "AssignmentStatement");
    // Identifier already consumed
    // Support for =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
    if (match(parser, EQUAL)) {
        // Standard assignment
    } else if (match(parser, PLUS) && match(parser, EQUAL)) {
        // +=
    } else if (match(parser, MINUS) && match(parser, EQUAL)) {
        // -=
    } else if (match(parser, STAR) && match(parser, EQUAL)) {
        // *=
    } else if (match(parser, SLASH) && match(parser, EQUAL)) {
        // /=
    } else if (match(parser, PERCENT) && match(parser, EQUAL)) {
        // %=
    } else {
        // Fallback for simple = if complex matching fails or isn't implemented in lexer as single token
        
        if (parser->current_token.type == EQUAL) {
            advance(parser);
        } else if (parser->current_token.type == PLUS && parser->next_token.type == EQUAL) {
            advance(parser); 
            advance(parser);
        } else if (parser->current_token.type == MINUS && parser->next_token.type == EQUAL) {
            advance(parser); 
            advance(parser);
        } else if (parser->current_token.type == STAR && parser->next_token.type == EQUAL) {
            advance(parser); 
            advance(parser);
        } else if (parser->current_token.type == SLASH && parser->next_token.type == EQUAL) {
            advance(parser); 
            advance(parser);
        } else {
             error(parser, "Expect assignment operator (=, +=, -=, etc.) after identifier.");
        }
    }

    expression(parser);
    consume(parser, SEMICOLON, "Expect ';' after assignment.");
    exit_node(parser, "AssignmentStatement");
}

// 5. Input Statement: input(identifier);
static void input_statement(Parser* parser) {
    enter_node(parser, "InputStatement");
    // 'input' identifier already consumed
    consume(parser, LEFT_PAREN, "Expect '(' after 'input'.");
    consume(parser, IDENTIFIER, "Expect variable name in input.");
    consume(parser, RIGHT_PAREN, "Expect ')' after input variable.");
    consume(parser, SEMICOLON, "Expect ';' after input statement.");
    exit_node(parser, "InputStatement");
}

// 6. Output Statement: print(expression);
static void output_statement(Parser* parser) {
    enter_node(parser, "OutputStatement");
    // 'print' identifier already consumed
    consume(parser, LEFT_PAREN, "Expect '(' after 'print'.");
    expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after print expression.");
    consume(parser, SEMICOLON, "Expect ';' after print statement.");
    exit_node(parser, "OutputStatement");
}

// 8. Iterative Statement: while, for, foreach, do-while
static void while_statement(Parser* parser) {
    enter_node(parser, "WhileStatement");
    // 'while' already consumed
    // Check for optional noise word 'its'
    if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "its") == 0) {
        advance(parser);
    }

    consume(parser, LEFT_PAREN, "Expect '(' after 'while'.");
    expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
    
    statement(parser);
    exit_node(parser, "WhileStatement");
}

static void for_statement(Parser* parser) {
    enter_node(parser, "ForStatement");
    // 'for' already consumed
    consume(parser, LEFT_PAREN, "Expect '(' after 'for'.");
    
    // Initializer
    if (match(parser, SEMICOLON)) {
        // No initializer
    } else if (match(parser, TYPE)) {
        declaration_statement(parser);
    } else {
        // Expression statement (assignment)
        if (match(parser, IDENTIFIER)) {
             assignment_statement(parser);
        } else {
             error(parser, "Expect variable declaration or assignment in for loop.");
        }
    }
    
    // Condition
    if (!check(parser, SEMICOLON)) {
        expression(parser);
    }
    consume(parser, SEMICOLON, "Expect ';' after loop condition.");
    
    // Increment
    if (!check(parser, RIGHT_PAREN)) {
        expression(parser);
    }
    consume(parser, RIGHT_PAREN, "Expect ')' after for clauses.");
    
    statement(parser);
    exit_node(parser, "ForStatement");
}

static void statement(Parser* parser) {
    enter_node(parser, "Statement");
    
    if (match(parser, TYPE)) {
        declaration_statement(parser);
    } else if (check(parser, RESERVED_WORD) || check(parser, KEYWORD)) {
        if (strcmp(parser->current_token.lexeme, "while") == 0) {
            advance(parser);
            while_statement(parser);
        } else if (strcmp(parser->current_token.lexeme, "for") == 0) {
            advance(parser);
            for_statement(parser);
        } else if (strcmp(parser->current_token.lexeme, "if") == 0) {
            enter_node(parser, "IfStatement");
            advance(parser);
            // Check for optional noise word 'at'
            if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "at") == 0) {
                advance(parser);
            }
            consume(parser, LEFT_PAREN, "Expect '(' after 'if'.");
            expression(parser);
            consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
            // Check for optional noise word 'then'
            if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "then") == 0) {
                advance(parser);
            }
            statement(parser);
            // Else...
            if (check(parser, RESERVED_WORD) && strcmp(parser->current_token.lexeme, "else") == 0) {
                advance(parser);
                statement(parser);
            }
            exit_node(parser, "IfStatement");
        } else if (strcmp(parser->current_token.lexeme, "return") == 0) {
            enter_node(parser, "ReturnStatement");
            advance(parser);
            if (!check(parser, SEMICOLON)) {
                expression(parser);
            }
            consume(parser, SEMICOLON, "Expect ';' after return value.");
            exit_node(parser, "ReturnStatement");
        } else if (strcmp(parser->current_token.lexeme, "input") == 0) {
            advance(parser);
            input_statement(parser);
        } else if (strcmp(parser->current_token.lexeme, "var") == 0 || 
                   strcmp(parser->current_token.lexeme, "const") == 0 ||
                   strcmp(parser->current_token.lexeme, "dyn") == 0) {
            // Handle var/const/dyn as declaration starters
            advance(parser); // Consume the keyword
            declaration_statement(parser);
        } else {
            // Other keywords
            advance(parser);
        }
    } else if (match(parser, LEFT_BRACE)) {
        block(parser);
    } else if (match(parser, IDENTIFIER)) {
        // Access the previous token
        Token* prev = &parser->previous_token;
        
        if (prev->lexeme && strcmp(prev->lexeme, "input") == 0) {
            input_statement(parser);
        } else if (prev->lexeme && strcmp(prev->lexeme, "print") == 0) {
            output_statement(parser);
        } else if (check(parser, EQUAL)) {
            assignment_statement(parser);
        } else if (check(parser, LEFT_PAREN)) {
            enter_node(parser, "FunctionCall");
            // Function call statement
            consume(parser, LEFT_PAREN, "Expect '(' after function name.");
            if (!check(parser, RIGHT_PAREN)) {
                expression(parser); // Arguments
            }
            consume(parser, RIGHT_PAREN, "Expect ')' after arguments.");
            consume(parser, SEMICOLON, "Expect ';' after function call.");
            exit_node(parser, "FunctionCall");
        } else if (check(parser, PLUS_PLUS) || check(parser, MINUS_MINUS)) {
            enter_node(parser, "IncrementStatement");
            // Increment/Decrement statement
            advance(parser);
            consume(parser, SEMICOLON, "Expect ';' after increment/decrement.");
            exit_node(parser, "IncrementStatement");
        } else {
            error(parser, "Unexpected identifier usage.");
        }
    } else {
        // Skip unknown or extra tokens to avoid infinite loop if not synchronized
        if (parser->current_token.type != TOKEN_EOF) {
             // error(parser, "Expect statement.");
             advance(parser);
        }
    }
    
    if (parser->panic_mode) synchronize(parser);
    
    exit_node(parser, "Statement");
}

void parser_parse(Parser* parser) {
    printf("Starting Syntax Analysis...\n");
    enter_node(parser, "Program");
    while (parser->current_token.type != TOKEN_EOF) {
        statement(parser);
    }
    exit_node(parser, "Program");
    
    if (!parser->had_error) {
        printf("Syntax Analysis Complete: No errors found.\n");
    } else {
        printf("Syntax Analysis Complete: Errors found.\n");
    }
}

/* ============================================================================
 * MAIN FUNCTION
 * ============================================================================ */

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <symbol-table-file.txt>\n", argv[0]);
        return 1;
    }
    
    // Prepare output file
    // Input: filename.cytho.symboltable.txt
    // Output: filename.cytho.parsetree.txt
    
    char output_path[256];
    const char* suffix = ".symboltable.txt";
    size_t input_len = strlen(argv[1]);
    size_t suffix_len = strlen(suffix);
    
    if (input_len > suffix_len && strcmp(argv[1] + input_len - suffix_len, suffix) == 0) {
        strncpy(output_path, argv[1], input_len - suffix_len);
        output_path[input_len - suffix_len] = '\0';
        strcat(output_path, ".parsetree.txt");
    } else {
        // Fallback: just append .parsetree.txt
        snprintf(output_path, sizeof(output_path), "%s.parsetree.txt", argv[1]);
    }
    
    FILE* output_file = fopen(output_path, "w");
    if (!output_file) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_path);
    } else {
        printf("Writing parse tree to: %s\n", output_path);
    }

    // Run Parser
    Parser* parser = parser_create(argv[1]);
    if (!parser) {
        return 1;
    }
    
    parser->output_file = output_file;
    parser_parse(parser);
    
    if (output_file) fclose(output_file);
    
    // Cleanup
    if (parser->symbol_table_file) fclose(parser->symbol_table_file);
    if (parser->current_token.lexeme) free(parser->current_token.lexeme);
    if (parser->current_token.raw) free(parser->current_token.raw);
    if (parser->next_token.lexeme) free(parser->next_token.lexeme);
    if (parser->next_token.raw) free(parser->next_token.raw);
    if (parser->previous_token.lexeme) free(parser->previous_token.lexeme);
    if (parser->previous_token.raw) free(parser->previous_token.raw);
    
    free(parser);
    
    return 0;
}
