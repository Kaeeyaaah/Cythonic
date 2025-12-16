#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>

/* ============================================================================
 * TOKEN TYPE DEFINITIONS
 * ============================================================================ */

typedef enum {
    // Keywords and Types
    KEYWORD,           // Contextual keywords (21 total)
    RESERVED_WORD,     // Reserved words (33 total)
    TYPE,              // Type keywords (7 total)
    IDENTIFIER,        // User-defined identifiers
    BOOLEAN_LITERAL,   // true, false
    NOISE_WORD,        // at, its, then (optional fillers)
    
    // Literals
    NUMBER,            // Integer, float, scientific notation
    STRING_LITERAL,    // "text" (allows unclosed strings)
    CHAR_LITERAL,      // 'c'
    
    // Arithmetic Operators
    PLUS,              // +
    MINUS,             // -
    STAR,              // *
    SLASH,             // /
    PERCENT,           // %
    PLUS_PLUS,         // ++
    MINUS_MINUS,       // --
    
    // Assignment
    EQUAL,             // =
    
    // Comparison Operators
    EQUAL_EQUAL,       // ==
    NOT_EQUAL,         // !=
    GREATER,           // >
    LESS,              // <
    GREATER_EQUAL,     // >=
    LESS_EQUAL,        // <=
    
    // Logical Operators
    AND_AND,           // &&
    OR_OR,             // ||
    NOT,               // !
    
    // Bitwise Operators
    AND,               // &
    OR,                // |
    XOR,               // ^
    TILDE,             // ~
    
    // Delimiters
    LEFT_PAREN,        // (
    RIGHT_PAREN,       // )
    LEFT_BRACE,        // {
    RIGHT_BRACE,       // }
    LEFT_BRACKET,      // [
    RIGHT_BRACKET,     // ]
    SEMICOLON,         // ;
    COMMA,             // ,
    DOT,               // .
    COLON,             // :
    QUESTION,          // ?
    
    // Other
    COMMENT,           // // or /* */
    INVALID,           // Invalid/unrecognized tokens (NOT ignored)
    TOKEN_EOF          // End of file
} TokenType;

/* ============================================================================
 * TOKEN STRUCTURE
 * ============================================================================ */

typedef struct {
    TokenType type;
    char* lexeme;      // Normalized (lowercase for identifiers/keywords)
    char* raw;         // Original text as written
    int line;
    int column;
} Token;

/* ============================================================================
 * LEXER STATE
 * ============================================================================ */

#define MAX_TOKENS 10000
#define MAX_LEXEME_LENGTH 256
#define IDENTIFIER_MAX_LENGTH 31

typedef struct {
    const char* source;
    int length;
    int index;
    int line;
    int column;
    Token tokens[MAX_TOKENS];
    int token_count;
} Lexer;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

Lexer* lexer_create(const char* source);
void lexer_run(Lexer* lexer);
const char* token_type_to_string(TokenType type);

#endif // LEXER_H
