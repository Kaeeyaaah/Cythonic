/*
 * Cythonic Lexical Analyzer - C Implementation
 * 
 * A production-ready lexer for the Cythonic language (.cytho files)
 * Implements DFA-based tokenization with explicit state machines
 * 
 * Features:
 * - Case-insensitive keywords (normalized to lowercase)
 * - 21 Contextual Keywords (KEYWORD)
 * - 33 Reserved Words (RESERVED_WORD)
 * - 3 Noise Words (NOISE_WORD)
 * - Full operator and delimiter recognition
 * - String literals without requiring closing quotes
 * - Invalid tokens recognized as INVALID type (not ignored)
 * - Comprehensive error reporting with line/column tracking
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "Lexer.h"

/* ============================================================================
 * KEYWORD TRIE (DFA) STRUCTURE
 * ============================================================================ */

#define TRIE_MAX_STATES 200

typedef struct {
    int transitions[26];  // a-z transitions
    bool is_accepting;
    TokenType accepting_type;
} TrieNode;

typedef struct {
    TrieNode nodes[TRIE_MAX_STATES];
    int node_count;
} KeywordTrie;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

static bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool is_identifier_start(char c) {
    return is_letter(c) || c == '_';
}

static bool is_identifier_char(char c) {
    return is_letter(c) || is_digit(c) || c == '_';
}

static char* str_duplicate(const char* str) {
    if (!str) return NULL;
    char* dup = malloc(strlen(str) + 1);
    if (dup) strcpy(dup, str);
    return dup;
}

static char* str_to_lower(const char* str) {
    if (!str) return NULL;
    char* lower = malloc(strlen(str) + 1);
    if (!lower) return NULL;
    for (int i = 0; str[i]; i++) {
        lower[i] = to_lower(str[i]);
    }
    lower[strlen(str)] = '\0';
    return lower;
}

/* ============================================================================
 * KEYWORD TRIE IMPLEMENTATION
 * ============================================================================ */

static KeywordTrie* trie_create() {
    KeywordTrie* trie = malloc(sizeof(KeywordTrie));
    trie->node_count = 1;
    
    // Initialize root node
    for (int i = 0; i < 26; i++) {
        trie->nodes[0].transitions[i] = -1;
    }
    trie->nodes[0].is_accepting = false;
    
    return trie;
}

static void trie_add(KeywordTrie* trie, const char* text, TokenType type) {
    int state = 0;
    
    for (int i = 0; text[i]; i++) {
        char lower = to_lower(text[i]);
        int index = lower - 'a';
        
        if (index < 0 || index >= 26) continue;
        
        if (trie->nodes[state].transitions[index] == -1) {
            int next = trie->node_count++;
            trie->nodes[state].transitions[index] = next;
            
            // Initialize new node
            for (int j = 0; j < 26; j++) {
                trie->nodes[next].transitions[j] = -1;
            }
            trie->nodes[next].is_accepting = false;
        }
        
        state = trie->nodes[state].transitions[index];
    }
    
    trie->nodes[state].is_accepting = true;
    trie->nodes[state].accepting_type = type;
}

static int trie_move(KeywordTrie* trie, int state, char c) {
    if (state < 0 || state >= trie->node_count) return -1;
    int index = c - 'a';
    if (index < 0 || index >= 26) return -1;
    return trie->nodes[state].transitions[index];
}

static bool trie_try_get_type(KeywordTrie* trie, int state, TokenType* out_type) {
    if (state >= 0 && state < trie->node_count && trie->nodes[state].is_accepting) {
        *out_type = trie->nodes[state].accepting_type;
        return true;
    }
    return false;
}

static KeywordTrie* initialize_keywords() {
    KeywordTrie* trie = trie_create();
    
    // Contextual Keywords (21)
    trie_add(trie, "and", KEYWORD);
    trie_add(trie, "args", KEYWORD);
    trie_add(trie, "async", KEYWORD);
    trie_add(trie, "dyn", KEYWORD);
    trie_add(trie, "get", KEYWORD);
    trie_add(trie, "global", KEYWORD);
    trie_add(trie, "init", KEYWORD);
    trie_add(trie, "let", KEYWORD);
    trie_add(trie, "nmof", KEYWORD);
    trie_add(trie, "nnull", KEYWORD);
    trie_add(trie, "or", KEYWORD);
    trie_add(trie, "rec", KEYWORD);
    trie_add(trie, "req", KEYWORD);
    trie_add(trie, "set", KEYWORD);
    trie_add(trie, "stc", KEYWORD);
    trie_add(trie, "str", TYPE);
    trie_add(trie, "struct", KEYWORD);
    trie_add(trie, "switch", KEYWORD);
    trie_add(trie, "this", KEYWORD);
    trie_add(trie, "val", KEYWORD);
    trie_add(trie, "var", KEYWORD);
    
    // Reserved Words (33)
    trie_add(trie, "as", RESERVED_WORD);
    trie_add(trie, "base", RESERVED_WORD);
    trie_add(trie, "bool", TYPE);
    trie_add(trie, "break", RESERVED_WORD);
    trie_add(trie, "case", RESERVED_WORD);
    trie_add(trie, "char", TYPE);
    trie_add(trie, "class", RESERVED_WORD);
    trie_add(trie, "default", RESERVED_WORD);
    trie_add(trie, "do", RESERVED_WORD);
    trie_add(trie, "double", TYPE);
    trie_add(trie, "else", RESERVED_WORD);
    trie_add(trie, "enum", RESERVED_WORD);
    trie_add(trie, "false", BOOLEAN_LITERAL);
    trie_add(trie, "for", RESERVED_WORD);
    trie_add(trie, "foreach", RESERVED_WORD);
    trie_add(trie, "if", RESERVED_WORD);
    trie_add(trie, "iface", RESERVED_WORD);
    trie_add(trie, "in", RESERVED_WORD);
    trie_add(trie, "int", TYPE);
    trie_add(trie, "new", RESERVED_WORD);
    trie_add(trie, "next", RESERVED_WORD);
    trie_add(trie, "nspace", RESERVED_WORD);
    trie_add(trie, "null", RESERVED_WORD);
    trie_add(trie, "num", TYPE);
    trie_add(trie, "priv", RESERVED_WORD);
    trie_add(trie, "prot", RESERVED_WORD);
    trie_add(trie, "pub", RESERVED_WORD);
    trie_add(trie, "rdo", RESERVED_WORD);
    trie_add(trie, "record", RESERVED_WORD);
    trie_add(trie, "return", RESERVED_WORD);
    trie_add(trie, "true", BOOLEAN_LITERAL);
    trie_add(trie, "use", RESERVED_WORD);
    trie_add(trie, "void", TYPE);
    trie_add(trie, "while", RESERVED_WORD);
    
    // Noise Words (3)
    trie_add(trie, "at", NOISE_WORD);
    trie_add(trie, "its", NOISE_WORD);
    trie_add(trie, "then", NOISE_WORD);
    
    return trie;
}

/* ============================================================================
 * LEXER IMPLEMENTATION
 * ============================================================================ */

Lexer* lexer_create(const char* source) {
    Lexer* lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->index = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->token_count = 0;
    return lexer;
}

static bool lexer_is_at_end(Lexer* lexer) {
    return lexer->index >= lexer->length;
}

static char lexer_peek(Lexer* lexer, int offset) {
    int pos = lexer->index + offset;
    if (pos >= lexer->length) return '\0';
    return lexer->source[pos];
}

static char lexer_current(Lexer* lexer) {
    return lexer_peek(lexer, 0);
}

static void lexer_advance(Lexer* lexer) {
    if (lexer->index < lexer->length) {
        if (lexer->source[lexer->index] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->index++;
    }
}

static void lexer_add_token(Lexer* lexer, TokenType type, const char* lexeme, const char* raw, int line, int col) {
    if (lexer->token_count >= MAX_TOKENS) {
        fprintf(stderr, "Error: Token limit exceeded\n");
        return;
    }
    
    Token* token = &lexer->tokens[lexer->token_count++];
    token->type = type;
    token->lexeme = str_duplicate(lexeme);
    token->raw = str_duplicate(raw);
    token->line = line;
    token->column = col;
}

/* ============================================================================
 * LEXING FUNCTIONS
 * ============================================================================ */

static void lex_single_line_comment(Lexer* lexer, int start_line, int start_col) {
    int start = lexer->index;
    lexer_advance(lexer); // Skip first /
    lexer_advance(lexer); // Skip second /
    
    while (!lexer_is_at_end(lexer) && lexer_current(lexer) != '\n') {
        lexer_advance(lexer);
    }
    
    int length = lexer->index - start;
    char* raw = malloc(length + 1);
    strncpy(raw, &lexer->source[start], length);
    raw[length] = '\0';
    
    // Lexeme is content without //
    char* lexeme = malloc(length - 1);
    strncpy(lexeme, &lexer->source[start + 2], length - 2);
    lexeme[length - 2] = '\0';
    
    lexer_add_token(lexer, COMMENT, lexeme, raw, start_line, start_col);
    
    free(lexeme);
    free(raw);
}

static void lex_multi_line_comment(Lexer* lexer, int start_line, int start_col) {
    int start = lexer->index;
    lexer_advance(lexer); // Skip /
    lexer_advance(lexer); // Skip *
    
    while (!lexer_is_at_end(lexer)) {
        if (lexer_current(lexer) == '*' && lexer_peek(lexer, 1) == '/') {
            lexer_advance(lexer); // Skip *
            lexer_advance(lexer); // Skip /
            break;
        }
        lexer_advance(lexer);
    }
    
    int length = lexer->index - start;
    char* raw = malloc(length + 1);
    strncpy(raw, &lexer->source[start], length);
    raw[length] = '\0';
    
    // Lexeme is content without /* */
    char* lexeme = malloc(length - 3);
    strncpy(lexeme, &lexer->source[start + 2], length - 4);
    lexeme[length - 4] = '\0';
    
    lexer_add_token(lexer, COMMENT, lexeme, raw, start_line, start_col);
    
    free(lexeme);
    free(raw);
}

static void lex_identifier_or_keyword(Lexer* lexer, KeywordTrie* trie, int start_line, int start_col) {
    int start = lexer->index;
    
    // Collect identifier characters
    while (!lexer_is_at_end(lexer) && is_identifier_char(lexer_current(lexer))) {
        lexer_advance(lexer);
    }
    
    int length = lexer->index - start;
    char* raw = malloc(length + 1);
    strncpy(raw, &lexer->source[start], length);
    raw[length] = '\0';
    
    // Determine token type
    TokenType type = IDENTIFIER;
    
    // Check if entire identifier is letters (keyword candidate)
    bool all_letters = true;
    for (int i = 0; i < length; i++) {
        if (!is_letter(raw[i])) {
            all_letters = false;
            break;
        }
    }
    
    if (all_letters) {
        // Try keyword recognition using trie
        int state = 0;
        int i = 0;
        for (; i < length; i++) {
            char lower = to_lower(raw[i]);
            state = trie_move(trie, state, lower);
            if (state == -1) break;
        }
        
        // Must match entire string AND end in accepting state
        if (i == length) {
            TokenType accepting_type;
            if (trie_try_get_type(trie, state, &accepting_type)) {
                type = accepting_type;
            }
        }
    }
    
    // Create lexeme (lowercase for identifiers/keywords, truncate to 31 chars)
    char* lexeme;
    if (type == IDENTIFIER) {
        lexeme = str_to_lower(raw);
        if (strlen(lexeme) > IDENTIFIER_MAX_LENGTH) {
            lexeme[IDENTIFIER_MAX_LENGTH] = '\0';
        }
    } else {
        lexeme = str_to_lower(raw);
    }
    
    lexer_add_token(lexer, type, lexeme, raw, start_line, start_col);
    
    free(lexeme);
    free(raw);
}

static void lex_number(Lexer* lexer, int start_line, int start_col) {
    int start = lexer->index;
    
    // Integer part
    while (!lexer_is_at_end(lexer) && is_digit(lexer_current(lexer))) {
        lexer_advance(lexer);
    }
    
    // Decimal part
    if (!lexer_is_at_end(lexer) && lexer_current(lexer) == '.' && is_digit(lexer_peek(lexer, 1))) {
        lexer_advance(lexer); // Skip .
        while (!lexer_is_at_end(lexer) && is_digit(lexer_current(lexer))) {
            lexer_advance(lexer);
        }
    }
    
    // Exponent part
    if (!lexer_is_at_end(lexer) && (lexer_current(lexer) == 'e' || lexer_current(lexer) == 'E')) {
        lexer_advance(lexer); // Skip e/E
        if (!lexer_is_at_end(lexer) && (lexer_current(lexer) == '+' || lexer_current(lexer) == '-')) {
            lexer_advance(lexer); // Skip +/-
        }
        while (!lexer_is_at_end(lexer) && is_digit(lexer_current(lexer))) {
            lexer_advance(lexer);
        }
    }
    
    int length = lexer->index - start;
    char* text = malloc(length + 1);
    strncpy(text, &lexer->source[start], length);
    text[length] = '\0';
    
    lexer_add_token(lexer, NUMBER, text, text, start_line, start_col);
    free(text);
}

static void lex_string_literal(Lexer* lexer, int start_line, int start_col) {
    int start = lexer->index;
    lexer_advance(lexer); // Skip opening "
    
    char buffer[MAX_LEXEME_LENGTH];
    int buf_pos = 0;
    
    // MODIFICATION: Allow unclosed strings (continue to end of line or file)
    while (!lexer_is_at_end(lexer) && lexer_current(lexer) != '\n') {
        char c = lexer_current(lexer);
        
        if (c == '"') {
            // Found closing quote
            lexer_advance(lexer);
            break;
        } else if (c == '\\' && !lexer_is_at_end(lexer)) {
            // Handle escape sequences
            lexer_advance(lexer);
            char next = lexer_current(lexer);
            lexer_advance(lexer);
            
            switch (next) {
                case 'n': buffer[buf_pos++] = '\n'; break;
                case 't': buffer[buf_pos++] = '\t'; break;
                case 'r': buffer[buf_pos++] = '\r'; break;
                case 'b': buffer[buf_pos++] = '\b'; break;
                case 'f': buffer[buf_pos++] = '\f'; break;
                case '0': buffer[buf_pos++] = '\0'; break;
                case '\\': buffer[buf_pos++] = '\\'; break;
                case '"': buffer[buf_pos++] = '"'; break;
                case '\'': buffer[buf_pos++] = '\''; break;
                default: buffer[buf_pos++] = next; break;
            }
        } else {
            buffer[buf_pos++] = c;
            lexer_advance(lexer);
        }
        
        if (buf_pos >= MAX_LEXEME_LENGTH - 1) break;
    }
    
    buffer[buf_pos] = '\0';
    
    int length = lexer->index - start;
    char* raw = malloc(length + 1);
    strncpy(raw, &lexer->source[start], length);
    raw[length] = '\0';
    
    lexer_add_token(lexer, STRING_LITERAL, buffer, raw, start_line, start_col);
    free(raw);
}

static void lex_char_literal(Lexer* lexer, int start_line, int start_col) {
    int start = lexer->index;
    lexer_advance(lexer); // Skip opening '
    
    char value = '\0';
    
    if (!lexer_is_at_end(lexer)) {
        char c = lexer_current(lexer);
        
        if (c == '\\') {
            // Escape sequence
            lexer_advance(lexer);
            if (!lexer_is_at_end(lexer)) {
                char next = lexer_current(lexer);
                lexer_advance(lexer);
                
                switch (next) {
                    case 'n': value = '\n'; break;
                    case 't': value = '\t'; break;
                    case 'r': value = '\r'; break;
                    case 'b': value = '\b'; break;
                    case 'f': value = '\f'; break;
                    case '0': value = '\0'; break;
                    case '\\': value = '\\'; break;
                    case '\'': value = '\''; break;
                    case '"': value = '"'; break;
                    default: value = next; break;
                }
            }
        } else {
            value = c;
            lexer_advance(lexer);
        }
    }
    
    // Skip closing ' if present
    if (!lexer_is_at_end(lexer) && lexer_current(lexer) == '\'') {
        lexer_advance(lexer);
    }
    
    int length = lexer->index - start;
    char* raw = malloc(length + 1);
    strncpy(raw, &lexer->source[start], length);
    raw[length] = '\0';
    
    char lexeme[2] = {value, '\0'};
    lexer_add_token(lexer, CHAR_LITERAL, lexeme, raw, start_line, start_col);
    free(raw);
}

static void lex_operator_or_delimiter(Lexer* lexer, int start_line, int start_col) {
    int start = lexer->index;
    char c = lexer_current(lexer);
    char next = lexer_peek(lexer, 1);
    
    TokenType type = INVALID;
    int advance_count = 1;
    
    // Two-character operators (longest match first)
    if (c == '+' && next == '+') { type = PLUS_PLUS; advance_count = 2; }
    else if (c == '-' && next == '-') { type = MINUS_MINUS; advance_count = 2; }
    else if (c == '=' && next == '=') { type = EQUAL_EQUAL; advance_count = 2; }
    else if (c == '!' && next == '=') { type = NOT_EQUAL; advance_count = 2; }
    else if (c == '>' && next == '=') { type = GREATER_EQUAL; advance_count = 2; }
    else if (c == '<' && next == '=') { type = LESS_EQUAL; advance_count = 2; }
    else if (c == '&' && next == '&') { type = AND_AND; advance_count = 2; }
    else if (c == '|' && next == '|') { type = OR_OR; advance_count = 2; }
    // Single-character operators and delimiters
    else if (c == '+') type = PLUS;
    else if (c == '-') type = MINUS;
    else if (c == '*') type = STAR;
    else if (c == '/') type = SLASH;
    else if (c == '%') type = PERCENT;
    else if (c == '=') type = EQUAL;
    else if (c == '>') type = GREATER;
    else if (c == '<') type = LESS;
    else if (c == '!') type = NOT;
    else if (c == '&') type = AND;
    else if (c == '|') type = OR;
    else if (c == '^') type = XOR;
    else if (c == '~') type = TILDE;
    else if (c == '(') type = LEFT_PAREN;
    else if (c == ')') type = RIGHT_PAREN;
    else if (c == '{') type = LEFT_BRACE;
    else if (c == '}') type = RIGHT_BRACE;
    else if (c == '[') type = LEFT_BRACKET;
    else if (c == ']') type = RIGHT_BRACKET;
    else if (c == ';') type = SEMICOLON;
    else if (c == ',') type = COMMA;
    else if (c == '.') type = DOT;
    else if (c == ':') type = COLON;
    else if (c == '?') type = QUESTION;
    
    for (int i = 0; i < advance_count; i++) {
        lexer_advance(lexer);
    }
    
    int length = lexer->index - start;
    char* text = malloc(length + 1);
    strncpy(text, &lexer->source[start], length);
    text[length] = '\0';
    
    lexer_add_token(lexer, type, text, text, start_line, start_col);
    free(text);
}

/* ============================================================================
 * MAIN LEXING FUNCTION
 * ============================================================================ */

static void lexer_lex(Lexer* lexer, KeywordTrie* trie) {
    while (!lexer_is_at_end(lexer)) {
        int start_line = lexer->line;
        int start_col = lexer->column;
        char current = lexer_current(lexer);
        
        // Skip whitespace
        if (is_whitespace(current)) {
            lexer_advance(lexer);
            continue;
        }
        
        // Comments
        if (current == '/' && lexer_peek(lexer, 1) == '/') {
            lex_single_line_comment(lexer, start_line, start_col);
            continue;
        }
        
        if (current == '/' && lexer_peek(lexer, 1) == '*') {
            lex_multi_line_comment(lexer, start_line, start_col);
            continue;
        }
        
        // Identifiers and keywords
        if (is_identifier_start(current)) {
            lex_identifier_or_keyword(lexer, trie, start_line, start_col);
            continue;
        }
        
        // Numbers
        if (is_digit(current)) {
            lex_number(lexer, start_line, start_col);
            continue;
        }
        
        // Numbers starting with decimal point
        if (current == '.' && is_digit(lexer_peek(lexer, 1))) {
            lex_number(lexer, start_line, start_col);
            continue;
        }
        
        // Character literals
        if (current == '\'') {
            lex_char_literal(lexer, start_line, start_col);
            continue;
        }
        
        // String literals
        if (current == '"') {
            lex_string_literal(lexer, start_line, start_col);
            continue;
        }
        
        // Operators and delimiters
        if (strchr("+-*/%=><>!&|^~(){}[];,.:?", current)) {
            lex_operator_or_delimiter(lexer, start_line, start_col);
            continue;
        }
        
        // MODIFICATION: Invalid characters are tokenized as INVALID (not ignored)
        char invalid_char[2] = {current, '\0'};
        lexer_add_token(lexer, INVALID, invalid_char, invalid_char, start_line, start_col);
        lexer_advance(lexer);
    }
    
    // Add EOF token
    lexer_add_token(lexer, TOKEN_EOF, "", "", lexer->line, lexer->column);
}

/* ============================================================================
 * TOKEN TYPE TO STRING
 * ============================================================================ */

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case KEYWORD: return "KEYWORD";
        case RESERVED_WORD: return "RESERVED_WORD";
        case TYPE: return "TYPE";
        case IDENTIFIER: return "IDENTIFIER";
        case BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        case NOISE_WORD: return "NOISE_WORD";
        case NUMBER: return "NUMBER";
        case STRING_LITERAL: return "STRING_LITERAL";
        case CHAR_LITERAL: return "CHAR_LITERAL";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case PERCENT: return "PERCENT";
        case PLUS_PLUS: return "PLUS_PLUS";
        case MINUS_MINUS: return "MINUS_MINUS";
        case EQUAL: return "EQUAL";
        case EQUAL_EQUAL: return "EQUAL_EQUAL";
        case NOT_EQUAL: return "NOT_EQUAL";
        case GREATER: return "GREATER";
        case LESS: return "LESS";
        case GREATER_EQUAL: return "GREATER_EQUAL";
        case LESS_EQUAL: return "LESS_EQUAL";
        case AND_AND: return "AND_AND";
        case OR_OR: return "OR_OR";
        case NOT: return "NOT";
        case AND: return "AND";
        case OR: return "OR";
        case XOR: return "XOR";
        case TILDE: return "TILDE";
        case LEFT_PAREN: return "LEFT_PAREN";
        case RIGHT_PAREN: return "RIGHT_PAREN";
        case LEFT_BRACE: return "LEFT_BRACE";
        case RIGHT_BRACE: return "RIGHT_BRACE";
        case LEFT_BRACKET: return "LEFT_BRACKET";
        case RIGHT_BRACKET: return "RIGHT_BRACKET";
        case SEMICOLON: return "SEMICOLON";
        case COMMA: return "COMMA";
        case DOT: return "DOT";
        case COLON: return "COLON";
        case QUESTION: return "QUESTION";
        case COMMENT: return "COMMENT";
        case INVALID: return "INVALID";
        case TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

/* ============================================================================
 * SYMBOL TABLE OUTPUT
 * ============================================================================ */

#ifdef LEXER_MAIN
static void escape_for_output(const char* source, char* dest, size_t dest_size) {
    size_t i = 0, j = 0;
    while (source[i] && j < dest_size - 1) {
        char c = source[i++];
        if (c == '\n') {
            if (j < dest_size - 2) { dest[j++] = '\\'; dest[j++] = 'n'; }
        } else if (c == '\r') {
            if (j < dest_size - 2) { dest[j++] = '\\'; dest[j++] = 'r'; }
        } else if (c == '\t') {
            if (j < dest_size - 2) { dest[j++] = '\\'; dest[j++] = 't'; }
        } else {
            dest[j++] = c;
        }
    }
    dest[j] = '\0';
}

static void write_symbol_table(Lexer* lexer, const char* output_path) {
    FILE* file = fopen(output_path, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create symbol table file '%s'\n", output_path);
        return;
    }
    
    fprintf(file, "CYTHONIC LEXICAL ANALYZER - SYMBOL TABLE\n");
    fprintf(file, "========================================\n\n");
    fprintf(file, "Total tokens: %d\n\n", lexer->token_count);
    fprintf(file, "LINE | COL | TYPE              | LEXEME                        | RAW\n");
    fprintf(file, "-----|-----|-------------------|-------------------------------|----------------------------------\n");
    
    char lexeme_buffer[1024];
    char raw_buffer[1024];

    for (int i = 0; i < lexer->token_count; i++) {
        Token* token = &lexer->tokens[i];
        if (token->type == TOKEN_EOF) continue;
        
        escape_for_output(token->lexeme, lexeme_buffer, sizeof(lexeme_buffer));
        escape_for_output(token->raw, raw_buffer, sizeof(raw_buffer));

        fprintf(file, "%4d | %3d | %-17s | %-29s | %s\n",
                token->line,
                token->column,
                token_type_to_string(token->type),
                lexeme_buffer,
                raw_buffer);
    }
    
    fprintf(file, "\nEND OF SYMBOL TABLE\n");
    fclose(file);
}
#endif

/* ============================================================================
 * MAIN FUNCTION - EXAMPLE USAGE
 * ============================================================================ */

void lexer_run(Lexer* lexer) {
    KeywordTrie* trie = initialize_keywords();
    lexer_lex(lexer, trie);
}

#ifdef LEXER_MAIN
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <input-file.cytho>\n", argv[0]);
        return 1;
    }
    
    // Read input file
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = malloc(file_size + 1);
    size_t bytes_read = fread(source, 1, file_size, file);
    source[bytes_read] = '\0';
    fclose(file);
    
    // Initialize keyword trie
    KeywordTrie* trie = initialize_keywords();
    
    // Create lexer and tokenize
    Lexer* lexer = lexer_create(source);
    lexer_lex(lexer, trie);
    
    /* 
    // Print tokens to console (Disabled for cleaner output)
    printf("[\n");
    for (int i = 0; i < lexer->token_count; i++) {
        Token* token = &lexer->tokens[i];
        if (token->type == TOKEN_EOF) continue;
        
        printf("  {\n");
        printf("    \"Type\": \"%s\",\n", token_type_to_string(token->type));
        printf("    \"Lexeme\": \"%s\",\n", token->lexeme);
        printf("    \"Raw\": \"%s\",\n", token->raw);
        printf("    \"Line\": %d,\n", token->line);
        printf("    \"Column\": %d\n", token->column);
        printf("  }%s\n", (i < lexer->token_count - 2) ? "," : "");
    }
    printf("]\n");
    */
    
    // Write symbol table
    char output_path[256];
    snprintf(output_path, sizeof(output_path), "%s.symboltable.txt", argv[1]);
    write_symbol_table(lexer, output_path);
    printf("\nSymbol table written to: %s\n", output_path);
    
    // Cleanup
    for (int i = 0; i < lexer->token_count; i++) {
        free(lexer->tokens[i].lexeme);
        free(lexer->tokens[i].raw);
    }
    free(lexer);
    free(trie);
    free(source);
    
    return 0;
}
#endif
