/*
 * Cythonic Compiler - Single Source Implementation
 * Combines Lexer and Recursive Descent Parser.
 * 
 * Features:
 * - DFA-based lexical analysis with Trie keyword recognition
 * - Recursive descent parser with operator precedence
 * - Compound assignment operators (+=, -=, *=, /=, %=)
 * - Logical operators (&&, ||) with proper precedence
 * - Error recovery and detailed parse tree generation
 * 
 * Usage: ./cythonic <source-file.cytho>
 * Output: <source-file.cytho.symboltable.txt> and <source-file.cytho.parsetree.txt>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* ============================================================================
 * TOKEN DEFINITIONS
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
    PLUS_EQUAL,        // +=
    MINUS_EQUAL,       // -=
    STAR_EQUAL,        // *=
    SLASH_EQUAL,       // /=
    PERCENT_EQUAL,     // %=
    
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

typedef struct {
    TokenType type;
    char* lexeme;      // Normalized (lowercase for identifiers/keywords)
    char* raw;         // Original text as written
    int line;
    int column;
} Token;

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
        case PLUS_EQUAL: return "PLUS_EQUAL";
        case MINUS_EQUAL: return "MINUS_EQUAL";
        case STAR_EQUAL: return "STAR_EQUAL";
        case SLASH_EQUAL: return "SLASH_EQUAL";
        case PERCENT_EQUAL: return "PERCENT_EQUAL";
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
 * LEXER IMPLEMENTATION
 * ============================================================================ */

#define TRIE_MAX_STATES 200
#define MAX_LEXEME_LENGTH 256
#define IDENTIFIER_MAX_LENGTH 31

typedef struct {
    int transitions[26];
    bool is_accepting;
    TokenType accepting_type;
} TrieNode;

typedef struct {
    TrieNode nodes[TRIE_MAX_STATES];
    int node_count;
} KeywordTrie;

typedef struct {
    const char* source;
    int length;
    int index;
    int line;
    int column;
    KeywordTrie* trie;
} Lexer;

// --- Trie Functions ---

static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static KeywordTrie* trie_create() {
    KeywordTrie* trie = malloc(sizeof(KeywordTrie));
    trie->node_count = 1;
    for (int i = 0; i < 26; i++) trie->nodes[0].transitions[i] = -1;
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
            for (int j = 0; j < 26; j++) trie->nodes[next].transitions[j] = -1;
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
    // Contextual Keywords
    trie_add(trie, "and", KEYWORD); trie_add(trie, "args", KEYWORD); trie_add(trie, "async", KEYWORD);
    trie_add(trie, "dyn", KEYWORD); trie_add(trie, "get", KEYWORD); trie_add(trie, "global", KEYWORD);
    trie_add(trie, "init", KEYWORD); trie_add(trie, "let", KEYWORD); trie_add(trie, "nmof", KEYWORD);
    trie_add(trie, "nnull", KEYWORD); trie_add(trie, "or", KEYWORD); trie_add(trie, "rec", KEYWORD);
    trie_add(trie, "req", KEYWORD); trie_add(trie, "set", KEYWORD); trie_add(trie, "stc", KEYWORD);
    trie_add(trie, "str", TYPE); trie_add(trie, "struct", KEYWORD); trie_add(trie, "switch", KEYWORD);
    trie_add(trie, "this", KEYWORD); trie_add(trie, "val", KEYWORD); trie_add(trie, "var", KEYWORD);
    trie_add(trie, "where", KEYWORD); trie_add(trie, "const", KEYWORD); trie_add(trie, "input", KEYWORD);
    trie_add(trie, "print", KEYWORD);
    // Reserved Words
    trie_add(trie, "as", RESERVED_WORD); trie_add(trie, "base", RESERVED_WORD); trie_add(trie, "bool", TYPE);
    trie_add(trie, "break", RESERVED_WORD); trie_add(trie, "case", RESERVED_WORD); trie_add(trie, "char", TYPE);
    trie_add(trie, "class", RESERVED_WORD); trie_add(trie, "default", RESERVED_WORD); trie_add(trie, "do", RESERVED_WORD);
    trie_add(trie, "double", TYPE); trie_add(trie, "else", RESERVED_WORD); trie_add(trie, "enum", RESERVED_WORD);
    trie_add(trie, "false", BOOLEAN_LITERAL); trie_add(trie, "for", RESERVED_WORD); trie_add(trie, "foreach", RESERVED_WORD);
    trie_add(trie, "if", RESERVED_WORD); trie_add(trie, "iface", RESERVED_WORD); trie_add(trie, "in", RESERVED_WORD);
    trie_add(trie, "int", TYPE); trie_add(trie, "new", RESERVED_WORD); trie_add(trie, "next", RESERVED_WORD);
    trie_add(trie, "nspace", RESERVED_WORD); trie_add(trie, "null", RESERVED_WORD); trie_add(trie, "num", TYPE);
    trie_add(trie, "priv", RESERVED_WORD); trie_add(trie, "prot", RESERVED_WORD); trie_add(trie, "pub", RESERVED_WORD);
    trie_add(trie, "rdo", RESERVED_WORD); trie_add(trie, "record", RESERVED_WORD); trie_add(trie, "return", RESERVED_WORD);
    trie_add(trie, "true", BOOLEAN_LITERAL); trie_add(trie, "use", RESERVED_WORD); trie_add(trie, "void", TYPE);
    trie_add(trie, "while", RESERVED_WORD);
    // Noise Words
    trie_add(trie, "at", NOISE_WORD); trie_add(trie, "its", NOISE_WORD); trie_add(trie, "then", NOISE_WORD);
    return trie;
}

// --- Lexer Helper Functions ---

static bool is_letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static bool is_identifier_start(char c) { return is_letter(c) || c == '_'; }
static bool is_identifier_char(char c) { return is_letter(c) || is_digit(c) || c == '_'; }

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
    for (int i = 0; str[i]; i++) lower[i] = to_lower(str[i]);
    lower[strlen(str)] = '\0';
    return lower;
}

Lexer* lexer_create(const char* source) {
    Lexer* lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->index = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->trie = initialize_keywords();
    return lexer;
}

static bool lexer_is_at_end(Lexer* lexer) { return lexer->index >= lexer->length; }

static char lexer_peek(Lexer* lexer, int offset) {
    int pos = lexer->index + offset;
    if (pos >= lexer->length) return '\0';
    return lexer->source[pos];
}

static char lexer_current(Lexer* lexer) { return lexer_peek(lexer, 0); }

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

static Token create_token(TokenType type, const char* lexeme, const char* raw, int line, int col) {
    Token token;
    token.type = type;
    token.lexeme = str_duplicate(lexeme);
    token.raw = str_duplicate(raw);
    token.line = line;
    token.column = col;
    return token;
}

// --- Lexing Logic ---

Token lexer_next_token(Lexer* lexer) {
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
            int start = lexer->index;
            lexer_advance(lexer); lexer_advance(lexer);
            while (!lexer_is_at_end(lexer) && lexer_current(lexer) != '\n') lexer_advance(lexer);
            
            int length = lexer->index - start;
            char* raw = malloc(length + 1);
            strncpy(raw, &lexer->source[start], length); raw[length] = '\0';
            char* lexeme = malloc(length - 1);
            strncpy(lexeme, &lexer->source[start + 2], length - 2); lexeme[length - 2] = '\0';
            
            Token t = create_token(COMMENT, lexeme, raw, start_line, start_col);
            free(raw); free(lexeme);
            return t;
        }
        if (current == '/' && lexer_peek(lexer, 1) == '*') {
            int start = lexer->index;
            lexer_advance(lexer); lexer_advance(lexer);
            while (!lexer_is_at_end(lexer)) {
                if (lexer_current(lexer) == '*' && lexer_peek(lexer, 1) == '/') {
                    lexer_advance(lexer); lexer_advance(lexer);
                    break;
                }
                lexer_advance(lexer);
            }
            int length = lexer->index - start;
            char* raw = malloc(length + 1);
            strncpy(raw, &lexer->source[start], length); raw[length] = '\0';
            char* lexeme = malloc(length - 3);
            strncpy(lexeme, &lexer->source[start + 2], length - 4); lexeme[length - 4] = '\0';
            
            Token t = create_token(COMMENT, lexeme, raw, start_line, start_col);
            free(raw); free(lexeme);
            return t;
        }

        // Identifiers and Keywords
        if (is_identifier_start(current)) {
            int start = lexer->index;
            while (!lexer_is_at_end(lexer) && is_identifier_char(lexer_current(lexer))) lexer_advance(lexer);
            
            int length = lexer->index - start;
            char* raw = malloc(length + 1);
            strncpy(raw, &lexer->source[start], length); raw[length] = '\0';
            
            TokenType type = IDENTIFIER;
            bool all_letters = true;
            for (int i = 0; i < length; i++) if (!is_letter(raw[i])) { all_letters = false; break; }
            
            if (all_letters) {
                int state = 0;
                int i = 0;
                for (; i < length; i++) {
                    state = trie_move(lexer->trie, state, to_lower(raw[i]));
                    if (state == -1) break;
                }
                if (i == length) trie_try_get_type(lexer->trie, state, &type);
            }
            
            char* lexeme = str_to_lower(raw);
            if (type == IDENTIFIER && strlen(lexeme) > IDENTIFIER_MAX_LENGTH) lexeme[IDENTIFIER_MAX_LENGTH] = '\0';
            
            Token t = create_token(type, lexeme, raw, start_line, start_col);
            free(raw); free(lexeme);
            return t;
        }

        // Numbers
        if (is_digit(current) || (current == '.' && is_digit(lexer_peek(lexer, 1)))) {
            int start = lexer->index;
            if (current == '.') lexer_advance(lexer);
            while (!lexer_is_at_end(lexer) && is_digit(lexer_current(lexer))) lexer_advance(lexer);
            if (current != '.' && !lexer_is_at_end(lexer) && lexer_current(lexer) == '.' && is_digit(lexer_peek(lexer, 1))) {
                lexer_advance(lexer);
                while (!lexer_is_at_end(lexer) && is_digit(lexer_current(lexer))) lexer_advance(lexer);
            }
            if (!lexer_is_at_end(lexer) && (lexer_current(lexer) == 'e' || lexer_current(lexer) == 'E')) {
                lexer_advance(lexer);
                if (!lexer_is_at_end(lexer) && (lexer_current(lexer) == '+' || lexer_current(lexer) == '-')) lexer_advance(lexer);
                while (!lexer_is_at_end(lexer) && is_digit(lexer_current(lexer))) lexer_advance(lexer);
            }
            int length = lexer->index - start;
            char* text = malloc(length + 1);
            strncpy(text, &lexer->source[start], length); text[length] = '\0';
            Token t = create_token(NUMBER, text, text, start_line, start_col);
            free(text);
            return t;
        }

        // String Literals
        if (current == '"') {
            int start = lexer->index;
            lexer_advance(lexer);
            char buffer[MAX_LEXEME_LENGTH];
            int buf_pos = 0;
            while (!lexer_is_at_end(lexer) && lexer_current(lexer) != '\n') {
                char c = lexer_current(lexer);
                if (c == '"') { lexer_advance(lexer); break; }
                else if (c == '\\' && !lexer_is_at_end(lexer)) {
                    lexer_advance(lexer);
                    char next = lexer_current(lexer);
                    lexer_advance(lexer);
                    switch (next) {
                        case 'n': buffer[buf_pos++] = '\n'; break;
                        case 't': buffer[buf_pos++] = '\t'; break;
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
            strncpy(raw, &lexer->source[start], length); raw[length] = '\0';
            Token t = create_token(STRING_LITERAL, buffer, raw, start_line, start_col);
            free(raw);
            return t;
        }

        // Char Literals
        if (current == '\'') {
            int start = lexer->index;
            lexer_advance(lexer);
            char value = '\0';
            if (!lexer_is_at_end(lexer)) {
                char c = lexer_current(lexer);
                if (c == '\\') {
                    lexer_advance(lexer);
                    if (!lexer_is_at_end(lexer)) {
                        char next = lexer_current(lexer);
                        lexer_advance(lexer);
                        switch (next) {
                            case 'n': value = '\n'; break;
                            case 't': value = '\t'; break;
                            default: value = next; break;
                        }
                    }
                } else {
                    value = c;
                    lexer_advance(lexer);
                }
            }
            if (!lexer_is_at_end(lexer) && lexer_current(lexer) == '\'') lexer_advance(lexer);
            int length = lexer->index - start;
            char* raw = malloc(length + 1);
            strncpy(raw, &lexer->source[start], length); raw[length] = '\0';
            char lexeme[2] = {value, '\0'};
            Token t = create_token(CHAR_LITERAL, lexeme, raw, start_line, start_col);
            free(raw);
            return t;
        }

        // Operators and Delimiters
        if (strchr("+-*/%=><>!&|^~(){}[];,.:?", current)) {
            int start = lexer->index;
            char next = lexer_peek(lexer, 1);
            TokenType type = INVALID;
            int advance_count = 1;
            
            // Two-character operators - check compound assignments first
            if (current == '+' && next == '=') { type = PLUS_EQUAL; advance_count = 2; }
            else if (current == '-' && next == '=') { type = MINUS_EQUAL; advance_count = 2; }
            else if (current == '*' && next == '=') { type = STAR_EQUAL; advance_count = 2; }
            else if (current == '/' && next == '=') { type = SLASH_EQUAL; advance_count = 2; }
            else if (current == '%' && next == '=') { type = PERCENT_EQUAL; advance_count = 2; }
            else if (current == '+' && next == '+') { type = PLUS_PLUS; advance_count = 2; }
            else if (current == '-' && next == '-') { type = MINUS_MINUS; advance_count = 2; }
            else if (current == '=' && next == '=') { type = EQUAL_EQUAL; advance_count = 2; }
            else if (current == '!' && next == '=') { type = NOT_EQUAL; advance_count = 2; }
            else if (current == '>' && next == '=') { type = GREATER_EQUAL; advance_count = 2; }
            else if (current == '<' && next == '=') { type = LESS_EQUAL; advance_count = 2; }
            else if (current == '&' && next == '&') { type = AND_AND; advance_count = 2; }
            else if (current == '|' && next == '|') { type = OR_OR; advance_count = 2; }
            else if (current == '+') type = PLUS;
            else if (current == '-') type = MINUS;
            else if (current == '*') type = STAR;
            else if (current == '/') type = SLASH;
            else if (current == '%') type = PERCENT;
            else if (current == '=') type = EQUAL;
            else if (current == '>') type = GREATER;
            else if (current == '<') type = LESS;
            else if (current == '!') type = NOT;
            else if (current == '&') type = AND;
            else if (current == '|') type = OR;
            else if (current == '^') type = XOR;
            else if (current == '~') type = TILDE;
            else if (current == '(') type = LEFT_PAREN;
            else if (current == ')') type = RIGHT_PAREN;
            else if (current == '{') type = LEFT_BRACE;
            else if (current == '}') type = RIGHT_BRACE;
            else if (current == '[') type = LEFT_BRACKET;
            else if (current == ']') type = RIGHT_BRACKET;
            else if (current == ';') type = SEMICOLON;
            else if (current == ',') type = COMMA;
            else if (current == '.') type = DOT;
            else if (current == ':') type = COLON;
            else if (current == '?') type = QUESTION;
            
            for (int i = 0; i < advance_count; i++) lexer_advance(lexer);
            int length = lexer->index - start;
            char* text = malloc(length + 1);
            strncpy(text, &lexer->source[start], length); text[length] = '\0';
            Token t = create_token(type, text, text, start_line, start_col);
            free(text);
            return t;
        }

        // Invalid
        char invalid_char[2] = {current, '\0'};
        Token t = create_token(INVALID, invalid_char, invalid_char, start_line, start_col);
        lexer_advance(lexer);
        return t;
    }
    
    return create_token(TOKEN_EOF, "", "", lexer->line, lexer->column);
}

/* ============================================================================
 * PARSER IMPLEMENTATION
 * ============================================================================ */

typedef struct {
    Lexer* lexer;
    Token current_token;
    Token next_token;
    Token previous_token;
    bool has_next_token;
    bool had_error;
    bool panic_mode;
    int indent_level;
    FILE* output_file;
} Parser;

static void advance(Parser* parser);
static void statement(Parser* parser);
static void expression(Parser* parser);

static void print_indent(Parser* parser) {
    if (!parser->output_file) return;
    for (int i = 0; i < parser->indent_level; i++) fprintf(parser->output_file, "  ");
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

Parser* parser_create(Lexer* lexer) {
    Parser* parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->indent_level = 0;
    parser->output_file = NULL;
    
    parser->current_token.type = INVALID;
    parser->current_token.lexeme = NULL;
    parser->current_token.raw = NULL;
    
    parser->previous_token.type = INVALID;
    parser->previous_token.lexeme = NULL;
    parser->previous_token.raw = NULL;
    
    // Prime the pump
    parser->next_token = lexer_next_token(parser->lexer);
    parser->has_next_token = true;
    
    advance(parser);
    return parser;
}

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    fprintf(stderr, "[line %d:%d] Error", token->line, token->column);
    if (token->type == TOKEN_EOF) fprintf(stderr, " at end");
    else if (token->type != INVALID) fprintf(stderr, " at '%s'", token->raw);
    fprintf(stderr, ": %s\n", message);
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->current_token, message);
}

static void advance(Parser* parser) {
    if (parser->previous_token.lexeme) free(parser->previous_token.lexeme);
    if (parser->previous_token.raw) free(parser->previous_token.raw);
    
    parser->previous_token = parser->current_token;
    parser->current_token = parser->next_token;
    
    if (parser->current_token.type != TOKEN_EOF) {
        parser->next_token = lexer_next_token(parser->lexer);
    } else {
        parser->next_token = create_token(TOKEN_EOF, "", "", parser->lexer->line, parser->lexer->column);
    }
    
    print_next_token(parser);
}

static bool check(Parser* parser, TokenType type) { return parser->current_token.type == type; }

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
            case KEYWORD: case RESERVED_WORD: case TYPE: return;
            default: ;
        }
        advance(parser);
    }
}

// --- Grammar Rules ---

static void block(Parser* parser) {
    enter_node(parser, "Block");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) statement(parser);
    consume(parser, RIGHT_BRACE, "Expect '}' after block.");
    exit_node(parser, "Block");
}

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
    while (match(parser, PLUS_PLUS) || match(parser, MINUS_MINUS)) {}
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
    while (match(parser, SLASH) || match(parser, STAR) || match(parser, PERCENT)) unary(parser);
    exit_node(parser, "Factor");
}

static void term(Parser* parser) {
    enter_node(parser, "Term");
    factor(parser);
    while (match(parser, MINUS) || match(parser, PLUS)) factor(parser);
    exit_node(parser, "Term");
}

static void comparison(Parser* parser) {
    enter_node(parser, "Comparison");
    term(parser);
    while (match(parser, GREATER) || match(parser, GREATER_EQUAL) ||
           match(parser, LESS) || match(parser, LESS_EQUAL)) term(parser);
    exit_node(parser, "Comparison");
}

static void equality(Parser* parser) {
    enter_node(parser, "Equality");
    comparison(parser);
    while (match(parser, NOT_EQUAL) || match(parser, EQUAL_EQUAL)) comparison(parser);
    exit_node(parser, "Equality");
}

static void logical_and(Parser* parser) {
    enter_node(parser, "LogicalAnd");
    equality(parser);
    while (match(parser, AND_AND)) equality(parser);
    exit_node(parser, "LogicalAnd");
}

static void logical_or(Parser* parser) {
    enter_node(parser, "LogicalOr");
    logical_and(parser);
    while (match(parser, OR_OR)) logical_and(parser);
    exit_node(parser, "LogicalOr");
}

static void expression(Parser* parser) {
    enter_node(parser, "Expression");
    logical_or(parser);
    exit_node(parser, "Expression");
}

static void declaration_statement(Parser* parser) {
    enter_node(parser, "DeclarationStatement");
    // After var/const/dyn, there might be an optional type
    if (match(parser, TYPE)) {}
    consume(parser, IDENTIFIER, "Expect variable name.");
    if (match(parser, EQUAL)) expression(parser);
    consume(parser, SEMICOLON, "Expect ';' after variable declaration.");
    exit_node(parser, "DeclarationStatement");
}

static void assignment_statement(Parser* parser) {
    enter_node(parser, "AssignmentStatement");
    // Check for compound assignment operators first
    if (match(parser, EQUAL)) {}
    else if (match(parser, PLUS_EQUAL)) {}
    else if (match(parser, MINUS_EQUAL)) {}
    else if (match(parser, STAR_EQUAL)) {}
    else if (match(parser, SLASH_EQUAL)) {}
    else if (match(parser, PERCENT_EQUAL)) {}
    else {
        error(parser, "Expect assignment operator (=, +=, -=, *=, /=, %=).");
    }
    expression(parser);
    consume(parser, SEMICOLON, "Expect ';' after assignment.");
    exit_node(parser, "AssignmentStatement");
}

static void input_statement(Parser* parser) {
    enter_node(parser, "InputStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'input'.");
    consume(parser, IDENTIFIER, "Expect variable name in input.");
    consume(parser, RIGHT_PAREN, "Expect ')' after input variable.");
    consume(parser, SEMICOLON, "Expect ';' after input statement.");
    exit_node(parser, "InputStatement");
}

static void output_statement(Parser* parser) {
    enter_node(parser, "OutputStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'print'.");
    expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after print expression.");
    consume(parser, SEMICOLON, "Expect ';' after print statement.");
    exit_node(parser, "OutputStatement");
}

static void while_statement(Parser* parser) {
    enter_node(parser, "WhileStatement");
    if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "its") == 0) advance(parser);
    consume(parser, LEFT_PAREN, "Expect '(' after 'while'.");
    expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
    statement(parser);
    exit_node(parser, "WhileStatement");
}

static void for_statement(Parser* parser) {
    enter_node(parser, "ForStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(parser, SEMICOLON)) {}
    else if (match(parser, TYPE)) declaration_statement(parser);
    else if (match(parser, IDENTIFIER)) assignment_statement(parser);
    else error(parser, "Expect variable declaration or assignment in for loop.");
    
    if (!check(parser, SEMICOLON)) expression(parser);
    consume(parser, SEMICOLON, "Expect ';' after loop condition.");
    
    if (!check(parser, RIGHT_PAREN)) expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after for clauses.");
    
    statement(parser);
    exit_node(parser, "ForStatement");
}

static void statement(Parser* parser) {
    enter_node(parser, "Statement");
    if (match(parser, TYPE)) declaration_statement(parser);
    else if (check(parser, RESERVED_WORD) || check(parser, KEYWORD)) {
        if (strcmp(parser->current_token.lexeme, "while") == 0) { advance(parser); while_statement(parser); }
        else if (strcmp(parser->current_token.lexeme, "for") == 0) { advance(parser); for_statement(parser); }
        else if (strcmp(parser->current_token.lexeme, "if") == 0) {
            enter_node(parser, "IfStatement");
            advance(parser);
            if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "at") == 0) advance(parser);
            consume(parser, LEFT_PAREN, "Expect '(' after 'if'.");
            expression(parser);
            consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
            if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "then") == 0) advance(parser);
            statement(parser);
            if (check(parser, RESERVED_WORD) && strcmp(parser->current_token.lexeme, "else") == 0) {
                advance(parser);
                statement(parser);
            }
            exit_node(parser, "IfStatement");
        } else if (strcmp(parser->current_token.lexeme, "return") == 0) {
            enter_node(parser, "ReturnStatement");
            advance(parser);
            if (!check(parser, SEMICOLON)) expression(parser);
            consume(parser, SEMICOLON, "Expect ';' after return value.");
            exit_node(parser, "ReturnStatement");
        } else if (strcmp(parser->current_token.lexeme, "input") == 0) {
            advance(parser);
            input_statement(parser);
        } else if (strcmp(parser->current_token.lexeme, "print") == 0) {
            advance(parser);
            output_statement(parser);
        } else if (strcmp(parser->current_token.lexeme, "var") == 0 || 
                   strcmp(parser->current_token.lexeme, "const") == 0 ||
                   strcmp(parser->current_token.lexeme, "dyn") == 0) {
            advance(parser);
            declaration_statement(parser);
        } else {
            advance(parser);
        }
    } else if (match(parser, LEFT_BRACE)) {
        block(parser);
    } else if (match(parser, IDENTIFIER)) {
        if (check(parser, EQUAL) || check(parser, PLUS_EQUAL) || check(parser, MINUS_EQUAL) ||
            check(parser, STAR_EQUAL) || check(parser, SLASH_EQUAL) || check(parser, PERCENT_EQUAL)) {
            assignment_statement(parser);
        }
        else if (check(parser, LEFT_PAREN)) {
            enter_node(parser, "FunctionCall");
            consume(parser, LEFT_PAREN, "Expect '(' after function name.");
            if (!check(parser, RIGHT_PAREN)) expression(parser);
            consume(parser, RIGHT_PAREN, "Expect ')' after arguments.");
            consume(parser, SEMICOLON, "Expect ';' after function call.");
            exit_node(parser, "FunctionCall");
        } else if (check(parser, PLUS_PLUS) || check(parser, MINUS_MINUS)) {
            enter_node(parser, "IncrementStatement");
            advance(parser);
            consume(parser, SEMICOLON, "Expect ';' after increment/decrement.");
            exit_node(parser, "IncrementStatement");
        } else {
            error(parser, "Unexpected identifier usage.");
        }
    } else {
        if (parser->current_token.type != TOKEN_EOF) advance(parser);
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
    if (!parser->had_error) printf("Syntax Analysis Complete: No errors found.\n");
    else printf("Syntax Analysis Complete: Errors found.\n");
}

/* ============================================================================
 * SYMBOL TABLE OUTPUT
 * ============================================================================ */

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
    fprintf(file, "LINE | COL | TYPE              | LEXEME                        | RAW\n");
    fprintf(file, "-----|-----|-------------------|-------------------------------|----------------------------------\n");
    
    char lexeme_buffer[1024];
    char raw_buffer[1024];
    int count = 0;

    Token token = lexer_next_token(lexer);
    while (token.type != TOKEN_EOF) {
        escape_for_output(token.lexeme, lexeme_buffer, sizeof(lexeme_buffer));
        escape_for_output(token.raw, raw_buffer, sizeof(raw_buffer));

        fprintf(file, "%4d | %3d | %-17s | %-29s | %s\n",
                token.line,
                token.column,
                token_type_to_string(token.type),
                lexeme_buffer,
                raw_buffer);
        
        if (token.lexeme) free(token.lexeme);
        if (token.raw) free(token.raw);
        
        count++;
        token = lexer_next_token(lexer);
    }
    if (token.lexeme) free(token.lexeme);
    if (token.raw) free(token.raw);
    
    fprintf(file, "\nTotal tokens: %d\n", count);
    fprintf(file, "END OF SYMBOL TABLE\n");
    fclose(file);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <source-file.cytho>\n", argv[0]);
        return 1;
    }

    // Read source file
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

    // 1. Generate Symbol Table
    char symbol_table_path[256];
    const char* suffix = ".cytho";
    size_t input_len = strlen(argv[1]);
    size_t suffix_len = strlen(suffix);
    
    if (input_len > suffix_len && strcmp(argv[1] + input_len - suffix_len, suffix) == 0) {
        strncpy(symbol_table_path, argv[1], input_len);
        symbol_table_path[input_len] = '\0';
        strcat(symbol_table_path, ".symboltable.txt");
    } else {
        snprintf(symbol_table_path, sizeof(symbol_table_path), "%s.symboltable.txt", argv[1]);
    }
    
    Lexer* lexer_for_table = lexer_create(source);
    write_symbol_table(lexer_for_table, symbol_table_path);
    printf("Symbol table written to: %s\n", symbol_table_path);
    
    // Cleanup first lexer (but keep source!)
    free(lexer_for_table->trie);
    free(lexer_for_table);

    // 2. Generate Parse Tree
    char parse_tree_path[256];
    if (input_len > suffix_len && strcmp(argv[1] + input_len - suffix_len, suffix) == 0) {
        strncpy(parse_tree_path, argv[1], input_len);
        parse_tree_path[input_len] = '\0';
        strcat(parse_tree_path, ".parsetree.txt");
    } else {
        snprintf(parse_tree_path, sizeof(parse_tree_path), "%s.parsetree.txt", argv[1]);
    }
    
    FILE* output_file = fopen(parse_tree_path, "w");
    if (!output_file) fprintf(stderr, "Error: Cannot create output file '%s'\n", parse_tree_path);
    else printf("Writing parse tree to: %s\n", parse_tree_path);

    // Run Parser with fresh lexer
    Lexer* lexer = lexer_create(source);
    Parser* parser = parser_create(lexer);
    parser->output_file = output_file;
    
    parser_parse(parser);

    // Cleanup
    if (output_file) fclose(output_file);
    if (parser->current_token.lexeme) free(parser->current_token.lexeme);
    if (parser->current_token.raw) free(parser->current_token.raw);
    if (parser->next_token.lexeme) free(parser->next_token.lexeme);
    if (parser->next_token.raw) free(parser->next_token.raw);
    if (parser->previous_token.lexeme) free(parser->previous_token.lexeme);
    if (parser->previous_token.raw) free(parser->previous_token.raw);
    free(parser);
    free(lexer->trie);
    free(lexer);
    free(source);

    return 0;
}
