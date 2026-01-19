/*
 * CYTHONIC COMPILER - Complete Language Specification
 * ====================================================
 * 
 * Case-insensitive scripting language with DFA lexer and recursive descent parser.
 * 
 * PROGRAM STRUCTURE:
 *   Script-style execution (no main function required)
 *   Statements execute sequentially from file top
 * 
 * IMPLEMENTED FEATURES:
 * 
 * 1. CONTEXTUAL KEYWORDS (23): and, args, async, dyn, get, global, init, input,
 *    let, nmof, nnull, or, print, rec, req, set, stc, str, struct, switch,
 *    this, val, var
 * 
 * 2. RESERVED WORDS (27): as, base, break, case, class, const, default, do,
 *    else, enum, for, foreach, if, iface, in, new, next, nspace, null, priv,
 *    prot, pub, rdo, record, return, use, while
 * 
 * 3. TYPES (5): bool, char, double, int, void
 * 
 * 4. BOOLEAN LITERALS (2): true, false
 * 
 * 5. NOISE WORDS (3): at, its, then (optional readability enhancers)
 * 
 * 4. OPERATORS:
 *    Arithmetic: addition, subtraction, multiplication, division, modulo
 *    Assignment: assign, compound assign (add, sub, mul, div, mod)
 *    Comparison: equality, inequality, relational (gt, lt, ge, le)
 *    Logical: and, or, not
 *    Bitwise: tokenized only (and, or, xor, not)
 * 
 * 5. EXPRESSION PRECEDENCE (9 levels):
 *    Primary - Postfix - Unary - Factor - Term - Comparison - And - Or - Assignment
 * 
 * 6. STATEMENTS: Declarations, Assignments, Input, Output, If-Else, While,
 *    For, Blocks, Increment, Decrement
 * 
 * 7. LITERALS: Numbers (int, float, scientific), Strings, Characters, Booleans
 * 
 * RESERVED FOR FUTURE (Tokenized only): class, struct, enum, record, iface,
 *    nspace, use, this, base, pub, priv, prot, rdo, switch, case, default,
 *    foreach, do, new, bitwise operations
 * 
 * COMPILER ARCHITECTURE: 200-state DFA Trie, longest-match tokenization,
 *    panic-mode error recovery, parse tree generation, symbol table tracking
 * 
 * USAGE: cythonic.exe source.cytho
 * OUTPUT: source.cytho.symboltable.txt, source.cytho.parsetree.txt
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
    KEYWORD,           // Contextual keywords
    RESERVED_WORD,     // Reserved words
    TYPE,              // Type keywords (5 total)
    IDENTIFIER,        // User-defined identifiers
    BOOLEAN_LITERAL,   // true, false (2 total)
    NOISE_WORD,        // at, its, then (3 total)

    // New Control Flow Tokens
    SWITCH, CASE, DEFAULT, BREAK, NEXT, DO,

    // New OOP & Structure Tokens
    CLASS, STRUCT, ENUM, RECORD,
    PUB, PRIV, PROT, REQ,
    GET, SET, INIT,

    // New Operator Tokens
    AS,
    
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
        case SWITCH: return "SWITCH";
        case CASE: return "CASE";
        case DEFAULT: return "DEFAULT";
        case BREAK: return "BREAK";
        case NEXT: return "NEXT";
        case DO: return "DO";
        case CLASS: return "CLASS";
        case STRUCT: return "STRUCT";
        case ENUM: return "ENUM";
        case RECORD: return "RECORD";
        case PUB: return "PUB";
        case PRIV: return "PRIV";
        case PROT: return "PROT";
        case REQ: return "REQ";
        case GET: return "GET";
        case SET: return "SET";
        case INIT: return "INIT";
        case AS: return "AS";
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

static TokenType token_type_from_string(const char* str) {
    if (strcmp(str, "KEYWORD") == 0) return KEYWORD;
    if (strcmp(str, "RESERVED_WORD") == 0) return RESERVED_WORD;
    if (strcmp(str, "TYPE") == 0) return TYPE;
    if (strcmp(str, "IDENTIFIER") == 0) return IDENTIFIER;
    if (strcmp(str, "BOOLEAN_LITERAL") == 0) return BOOLEAN_LITERAL;
    if (strcmp(str, "NOISE_WORD") == 0) return NOISE_WORD;
    if (strcmp(str, "SWITCH") == 0) return SWITCH;
    if (strcmp(str, "CASE") == 0) return CASE;
    if (strcmp(str, "DEFAULT") == 0) return DEFAULT;
    if (strcmp(str, "BREAK") == 0) return BREAK;
    if (strcmp(str, "NEXT") == 0) return NEXT;
    if (strcmp(str, "DO") == 0) return DO;
    if (strcmp(str, "CLASS") == 0) return CLASS;
    if (strcmp(str, "STRUCT") == 0) return STRUCT;
    if (strcmp(str, "ENUM") == 0) return ENUM;
    if (strcmp(str, "RECORD") == 0) return RECORD;
    if (strcmp(str, "PUB") == 0) return PUB;
    if (strcmp(str, "PRIV") == 0) return PRIV;
    if (strcmp(str, "PROT") == 0) return PROT;
    if (strcmp(str, "REQ") == 0) return REQ;
    if (strcmp(str, "GET") == 0) return GET;
    if (strcmp(str, "SET") == 0) return SET;
    if (strcmp(str, "INIT") == 0) return INIT;
    if (strcmp(str, "AS") == 0) return AS;
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
    if (strcmp(str, "PLUS_EQUAL") == 0) return PLUS_EQUAL;
    if (strcmp(str, "MINUS_EQUAL") == 0) return MINUS_EQUAL;
    if (strcmp(str, "STAR_EQUAL") == 0) return STAR_EQUAL;
    if (strcmp(str, "SLASH_EQUAL") == 0) return SLASH_EQUAL;
    if (strcmp(str, "PERCENT_EQUAL") == 0) return PERCENT_EQUAL;
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
    if (strcmp(str, "EOF") == 0) return TOKEN_EOF;
    return INVALID;
}

/* ============================================================================
 * SYMBOL TABLE INPUT READER
 * ============================================================================ */

static Token create_token(TokenType type, const char* lexeme, const char* raw, int line, int col);

typedef struct {
    Token* tokens;
    int count;
    int capacity;
} TokenList;

static void token_list_add(TokenList* list, Token token) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity < 8 ? 8 : list->capacity * 2;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(Token));
    }
    list->tokens[list->count++] = token;
}

static void unescape_string(const char* src, char* dest) {
    int i = 0, j = 0;
    while (src[i]) {
        if (src[i] == '\\' && src[i+1]) {
            i++;
            switch(src[i]) {
                case 'n': dest[j++] = '\n'; break;
                case 'r': dest[j++] = '\r'; break;
                case 't': dest[j++] = '\t'; break;
                default: dest[j++] = src[i]; break;
            }
        } else {
            dest[j++] = src[i];
        }
        i++;
    }
    dest[j] = '\0';
}

static TokenList read_tokens_from_symbol_table(const char* path) {
    TokenList list = {0};
    list.tokens = NULL;
    list.count = 0;
    list.capacity = 0;
    
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Error: Could not open symbol table file '%s'\n", path);
        return list;
    }

    char line[2048];
    for(int i=0; i<4; i++) { if(!fgets(line, sizeof(line), file)) break; } // Skip header

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Total tokens:", 13) == 0) break;
        if (strncmp(line, "END OF SYMBOL TABLE", 19) == 0) break;
        
        char* p1 = strchr(line, '|');
        char* p2 = p1 ? strchr(p1+1, '|') : NULL;
        char* p3 = p2 ? strchr(p2+1, '|') : NULL;
        char* p4 = p3 ? strchr(p3+1, '|') : NULL;
        
        if (!p1 || !p2 || !p3 || !p4) continue;
        
        *p1 = 0; *p2 = 0; *p3 = 0; *p4 = 0;
        
        int line_num = atoi(line);
        int col = atoi(p1 + 1);
        
        char type_str[64];
        char lexeme_str[512];
        char raw_str[1024];

        strncpy(type_str, p2 + 1, sizeof(type_str)-1); type_str[sizeof(type_str)-1] = 0;
        char* t = type_str; while(*t == ' ') t++;
        char* e = t + strlen(t) - 1; while(e >= t && (*e == ' ' || *e == '\n')) *e-- = 0;
        memmove(type_str, t, strlen(t) + 1);

        strncpy(lexeme_str, p3 + 1, sizeof(lexeme_str)-1); lexeme_str[sizeof(lexeme_str)-1] = 0;
        t = lexeme_str; while(*t == ' ') t++;
        e = t + strlen(t) - 1; while(e >= t && *e == ' ') *e-- = 0; 
        memmove(lexeme_str, t, strlen(t) + 1);

        strncpy(raw_str, p4 + 1, sizeof(raw_str)-1); raw_str[sizeof(raw_str)-1] = 0;
        t = raw_str; while(*t == ' ') t++;
        e = t + strlen(t) - 1; while(e >= t && (*e == ' ' || *e == '\n')) *e-- = 0;
        memmove(raw_str, t, strlen(t) + 1);
        
        TokenType type = token_type_from_string(type_str);
        
        if (type != COMMENT) { // FILTER COMMENTS from Parser Input
             char* final_lexeme = malloc(strlen(lexeme_str) + 1);
             unescape_string(lexeme_str, final_lexeme);
             
             char* final_raw = malloc(strlen(raw_str) + 1);
             unescape_string(raw_str, final_raw);

             Token token = create_token(type, final_lexeme, final_raw, line_num, col);
             free(final_lexeme);
             free(final_raw); // create_token duplicates them
             token_list_add(&list, token);
        }
    }
    fclose(file);
    return list;
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
    trie_add(trie, "dyn", KEYWORD); trie_add(trie, "global", KEYWORD);
    trie_add(trie, "input", KEYWORD); trie_add(trie, "let", KEYWORD);
    trie_add(trie, "nmof", KEYWORD); trie_add(trie, "nnull", KEYWORD); trie_add(trie, "or", KEYWORD);
    trie_add(trie, "print", KEYWORD); trie_add(trie, "rec", KEYWORD);
    trie_add(trie, "stc", KEYWORD); trie_add(trie, "str", KEYWORD);
    trie_add(trie, "this", KEYWORD);
    trie_add(trie, "val", KEYWORD); trie_add(trie, "var", KEYWORD);

    // New Token Mappings
    trie_add(trie, "switch", SWITCH);
    trie_add(trie, "case", CASE);
    trie_add(trie, "default", DEFAULT);
    trie_add(trie, "break", BREAK);
    trie_add(trie, "next", NEXT);
    trie_add(trie, "do", DO);
    trie_add(trie, "as", AS);
    trie_add(trie, "class", CLASS);
    trie_add(trie, "struct", STRUCT);
    trie_add(trie, "enum", ENUM);
    trie_add(trie, "record", RECORD);
    trie_add(trie, "pub", PUB);
    trie_add(trie, "priv", PRIV);
    trie_add(trie, "prot", PROT);
    trie_add(trie, "req", REQ);
    trie_add(trie, "get", GET);
    trie_add(trie, "set", SET);
    trie_add(trie, "init", INIT);

    // Reserved Words
    trie_add(trie, "base", RESERVED_WORD);
    trie_add(trie, "const", RESERVED_WORD);
    trie_add(trie, "else", RESERVED_WORD);
    trie_add(trie, "for", RESERVED_WORD); trie_add(trie, "foreach", RESERVED_WORD);
    trie_add(trie, "if", RESERVED_WORD); trie_add(trie, "iface", RESERVED_WORD); trie_add(trie, "in", RESERVED_WORD);
    trie_add(trie, "new", RESERVED_WORD);
    trie_add(trie, "nspace", RESERVED_WORD);
    trie_add(trie, "null", RESERVED_WORD);
    trie_add(trie, "rdo", RESERVED_WORD);
    trie_add(trie, "return", RESERVED_WORD); trie_add(trie, "use", RESERVED_WORD); trie_add(trie, "while", RESERVED_WORD);
    // Types (5 total) - Per PPL Project Proposal Group 8
    trie_add(trie, "bool", TYPE); trie_add(trie, "char", TYPE); trie_add(trie, "double", TYPE);
    trie_add(trie, "int", TYPE); trie_add(trie, "void", TYPE);
    // Boolean Literals (2 total)
    trie_add(trie, "false", BOOLEAN_LITERAL); trie_add(trie, "true", BOOLEAN_LITERAL);
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
 * INTERPRETER / EVALUATOR DEFINITIONS
 * ============================================================================ */

typedef enum { VAL_INT, VAL_DOUBLE, VAL_BOOL, VAL_STRING, VAL_CHAR, VAL_VOID, VAL_NULL } ValueType;

typedef struct {
    ValueType type;
    union {
        int int_val;
        double double_val;
        bool bool_val;
        char* string_val;
        char char_val;
    } as;
} Value;

typedef struct Env {
    char* name;
    Value value;
    bool is_const;
    struct Env* next;
} Env;

static Value make_int(int v) { Value val; val.type = VAL_INT; val.as.int_val = v; return val; }
static Value make_double(double v) { Value val; val.type = VAL_DOUBLE; val.as.double_val = v; return val; }
static Value make_bool(bool v) { Value val; val.type = VAL_BOOL; val.as.bool_val = v; return val; }
static Value make_string(const char* v) { 
    Value val; val.type = VAL_STRING; 
    val.as.string_val = malloc(strlen(v) + 1); 
    strcpy(val.as.string_val, v); 
    return val; 
}
static Value make_char(char v) { Value val; val.type = VAL_CHAR; val.as.char_val = v; return val; }
static Value make_void() { Value val; val.type = VAL_VOID; return val; }
static Value make_null() { Value val; val.type = VAL_NULL; return val; }

static void free_value(Value v) {
    if (v.type == VAL_STRING && v.as.string_val) free(v.as.string_val);
}

static Env* env_create() { return NULL; }

static void env_define(Env** env, const char* name, Value value, bool is_const) {
    Env* start = *env;
    // Check if exists
    Env* current = start;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            free_value(current->value);
            current->value = value; // Overwrite definition (allows shadowing logic or re-use in simple scope)
            current->is_const = is_const;
            return;
        }
        current = current->next;
    }
    // New
    Env* node = malloc(sizeof(Env));
    node->name = strdup(name);
    node->value = value;
    node->is_const = is_const;
    node->next = *env;
    *env = node;
}

static bool env_assign(Env* env, const char* name, Value value) {
    while (env) {
        if (strcmp(env->name, name) == 0) {
            if (env->is_const) return false;
            // Free old string if necessary
            if (env->value.type == VAL_STRING) free(env->value.as.string_val);
            env->value = value;
            return true;
        }
        env = env->next;
    }
    return false;
}

static bool env_get(Env* env, const char* name, Value* out) {
    while (env) {
        if (strcmp(env->name, name) == 0) { // Simple linear search linear scope
             // Deep copy value for usage
             *out = env->value;
             if (out->type == VAL_STRING) out->as.string_val = strdup(env->value.as.string_val);
             return true;
        }
        env = env->next;
    }
    return false;
}

static Value val_add(Value a, Value b) {
    if(a.type == VAL_INT && b.type == VAL_INT) return make_int(a.as.int_val + b.as.int_val);
    if(a.type == VAL_DOUBLE || b.type == VAL_DOUBLE) {
        double d1 = (a.type == VAL_INT) ? (double)a.as.int_val : a.as.double_val;
        double d2 = (b.type == VAL_INT) ? (double)b.as.int_val : b.as.double_val;
        return make_double(d1 + d2);
    }
    if(a.type == VAL_STRING) {
        // Concat
        // TODO: Handle string concat
    }
    return make_int(0); 
}
static Value val_sub(Value a, Value b) {
    double d1 = (a.type == VAL_INT) ? (double)a.as.int_val : a.as.double_val;
    double d2 = (b.type == VAL_INT) ? (double)b.as.int_val : b.as.double_val;
    if(a.type == VAL_INT && b.type == VAL_INT) return make_int(a.as.int_val - b.as.int_val);
    return make_double(d1 - d2);
}
static Value val_mul(Value a, Value b) {
    double d1 = (a.type == VAL_INT) ? (double)a.as.int_val : a.as.double_val;
    double d2 = (b.type == VAL_INT) ? (double)b.as.int_val : b.as.double_val;
    if(a.type == VAL_INT && b.type == VAL_INT) return make_int(a.as.int_val * b.as.int_val);
    return make_double(d1 * d2);
}
static Value val_div(Value a, Value b) {
    double d1 = (a.type == VAL_INT) ? (double)a.as.int_val : a.as.double_val;
    double d2 = (b.type == VAL_INT) ? (double)b.as.int_val : b.as.double_val;
    if(d2 == 0) return make_int(0); // Error
    if(a.type == VAL_INT && b.type == VAL_INT) return make_int(a.as.int_val / b.as.int_val);
    return make_double(d1 / d2);
}

/* ============================================================================
 * PARSER IMPLEMENTATION
 * ============================================================================ */

typedef struct {
    TokenList* token_list;
    int current_index;
    Token current_token;
    Token next_token;
    Token previous_token;
    bool has_next_token;
    bool had_error;
    bool panic_mode;
    int indent_level;
    FILE* output_file;
    
    // Evaluator
    Env* env;
    bool executing;
    bool trace_parse; // If true, write to output_file
    char last_id[64];
} Parser;

static void advance(Parser* parser);
static void statement(Parser* parser);
static Value expression(Parser* parser);

static void print_indent(Parser* parser) {
    if (!parser->output_file || !parser->trace_parse) return;
    for (int i = 0; i < parser->indent_level; i++) fprintf(parser->output_file, "  ");
}

static void enter_node(Parser* parser, const char* name) {
    if (parser->output_file && parser->trace_parse) {
        print_indent(parser);
        fprintf(parser->output_file, "Enter <%s>\n", name);
    }
    parser->indent_level++;
}

static void exit_node(Parser* parser, const char* name) {
    parser->indent_level--;
    if (!parser->output_file || !parser->trace_parse) return;
    print_indent(parser);
    fprintf(parser->output_file, "Exit <%s>\n", name);
}

static void print_next_token(Parser* parser) {

}

Parser* parser_create(TokenList* token_list) {
    Parser* parser = malloc(sizeof(Parser));
    parser->token_list = token_list;
    parser->current_index = 0;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->indent_level = 0;
    parser->output_file = NULL;
    
    parser->env = NULL;
    parser->executing = true;
    parser->trace_parse = true;

    parser->current_token.type = INVALID;
    parser->current_token.lexeme = NULL;
    parser->current_token.raw = NULL;
    
    parser->previous_token.type = INVALID;
    parser->previous_token.lexeme = NULL;
    parser->previous_token.raw = NULL;
    
    // Prime the pump
    if (parser->token_list->count > 0) {
        parser->next_token = parser->token_list->tokens[parser->current_index++];
    } else {
        parser->next_token = create_token(TOKEN_EOF, "", "", 0, 0);
    }
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
    // Note: We do not free lexemes here because they are owned by TokenList
    
    parser->previous_token = parser->current_token;
    parser->current_token = parser->next_token;
    
    if (parser->current_token.type != TOKEN_EOF) {
        if (parser->current_index < parser->token_list->count) {
            parser->next_token = parser->token_list->tokens[parser->current_index++];
        } else {
             // Create a dummy EOF token if we run out
            parser->next_token = create_token(TOKEN_EOF, "", "", 0, 0);
        }
    } else {
        // Keep returning EOF
        parser->next_token = create_token(TOKEN_EOF, "", "", 0, 0);
    }
    
    if (parser->output_file && parser->trace_parse) {
        print_indent(parser);
        fprintf(parser->output_file, "Next token is: %s Next lexeme is %s\n", 
            token_type_to_string(parser->current_token.type), 
            parser->current_token.lexeme ? parser->current_token.lexeme : "");
    }
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

static Value primary(Parser* parser) {
    enter_node(parser, "Primary");
    if (match(parser, NUMBER)) { 
        Value v;
        if (strchr(parser->previous_token.lexeme, '.') || strchr(parser->previous_token.lexeme, 'e')) 
             v = make_double(atof(parser->previous_token.lexeme));
        else v = make_int(atoi(parser->previous_token.lexeme));
        exit_node(parser, "Primary"); return v; 
    }
    if (match(parser, STRING_LITERAL)) { 
        Value v = make_string(parser->previous_token.lexeme);
        exit_node(parser, "Primary"); return v; 
    }
    if (match(parser, CHAR_LITERAL)) { 
        Value v = make_char(parser->previous_token.lexeme[0]);
        exit_node(parser, "Primary"); return v; 
    }
    if (match(parser, BOOLEAN_LITERAL)) { 
        Value v = make_bool(strcmp(parser->previous_token.lexeme, "true") == 0);
        exit_node(parser, "Primary"); return v; 
    }
    if (match(parser, IDENTIFIER)) {
        strncpy(parser->last_id, parser->previous_token.lexeme, 63);
        Value v = make_int(0);
        if (parser->executing) {
            if(!env_get(parser->env, parser->previous_token.lexeme, &v)) {
                // Default 0
            }
        }
        exit_node(parser, "Primary"); return v;
    }
    if (match(parser, KEYWORD)) {
        strncpy(parser->last_id, parser->previous_token.lexeme, 63);
        Value v = make_int(0);
        if (parser->executing) env_get(parser->env, parser->previous_token.lexeme, &v); 
        exit_node(parser, "Primary"); return v; 
    }
    
    if (match(parser, LEFT_PAREN)) {
        Value v = expression(parser);
        consume(parser, RIGHT_PAREN, "Expect ')' after expression.");
        exit_node(parser, "Primary");
        return v;
    }
    error(parser, "Expect expression.");
    exit_node(parser, "Primary");
    return make_null();
}

static Value postfix(Parser* parser) {
    enter_node(parser, "Prefix/Postfix");
    Value v = make_null();
    if (match(parser, PLUS_PLUS)) {
        v = postfix(parser);
        if (parser->executing) {
            v = val_add(v, make_int(1));
            env_assign(parser->env, parser->last_id, v); 
        }
    } else if (match(parser, MINUS_MINUS)) {
        v = postfix(parser);
        if (parser->executing) {
            v = val_sub(v, make_int(1));
            env_assign(parser->env, parser->last_id, v);
        }
    } else {
        v = primary(parser);
        while (check(parser, PLUS_PLUS) || check(parser, MINUS_MINUS)) {
            bool inc = check(parser, PLUS_PLUS);
            advance(parser);
            if (parser->executing) {
                Value old = v;
                Value new_val = inc ? val_add(v, make_int(1)) : val_sub(v, make_int(1));
                env_assign(parser->env, parser->last_id, new_val);
                v = old; 
            }
        }
    }
    exit_node(parser, "Prefix/Postfix");
    return v;
}

static Value unary(Parser* parser) {
    enter_node(parser, "Unary");
    if (match(parser, NOT)) {
        Value v = unary(parser);
        if(parser->executing && v.type == VAL_BOOL) v.as.bool_val = !v.as.bool_val;
        exit_node(parser, "Unary");
        return v;
    }
    if (match(parser, MINUS)) {
        Value v = unary(parser);
        if(parser->executing) {
             if (v.type == VAL_INT) v.as.int_val = -v.as.int_val;
             else if (v.type == VAL_DOUBLE) v.as.double_val = -v.as.double_val;
        }
        exit_node(parser, "Unary");
        return v;
    }
    Value v = postfix(parser);
    exit_node(parser, "Unary");
    return v;
}

static Value factor(Parser* parser) {
    enter_node(parser, "Factor");
    Value lhs = unary(parser);
    while (check(parser, SLASH) || check(parser, STAR) || check(parser, PERCENT)) {
        TokenType op = parser->current_token.type;
        advance(parser);
        Value rhs = unary(parser);
        if(parser->executing) {
            if(op == PLUS) lhs = val_add(lhs, rhs); 
            if(op == STAR) lhs = val_mul(lhs, rhs);
            if(op == SLASH) lhs = val_div(lhs, rhs);
            if(op == PERCENT) {
                 if (lhs.type == VAL_INT && rhs.type == VAL_INT) lhs = make_int(lhs.as.int_val % rhs.as.int_val);
            }
        }
    }
    exit_node(parser, "Factor");
    return lhs;
}

static Value term(Parser* parser) {
    enter_node(parser, "Term");
    Value lhs = factor(parser);
    while (check(parser, MINUS) || check(parser, PLUS)) {
        TokenType op = parser->current_token.type;
        advance(parser);
        Value rhs = factor(parser);
        if(parser->executing) {
            if(op == PLUS) lhs = val_add(lhs, rhs);
            if(op == MINUS) lhs = val_sub(lhs, rhs);
        }
    }
    exit_node(parser, "Term");
    return lhs;
}

static Value type_conversion(Parser* parser) {
    enter_node(parser, "TypeConversion");
    Value v = term(parser);
    while (match(parser, AS)) {
        consume(parser, TYPE, "Expect type after 'as'.");
        // Implementation of cast? For now skip.
    }
    exit_node(parser, "TypeConversion");
    return v;
}

static Value comparison(Parser* parser) {
    enter_node(parser, "Comparison");
    Value lhs = type_conversion(parser);
    while (check(parser, GREATER) || check(parser, GREATER_EQUAL) ||
           check(parser, LESS) || check(parser, LESS_EQUAL)) {
        TokenType op = parser->current_token.type;
        advance(parser);
        Value rhs = type_conversion(parser);
        if (parser->executing) {
             bool res = false;
             // Naive comparison (assume int for simplicity or double)
             double d1 = (lhs.type == VAL_INT) ? lhs.as.int_val : lhs.as.double_val;
             double d2 = (rhs.type == VAL_INT) ? rhs.as.int_val : rhs.as.double_val;
             if (op == GREATER) res = d1 > d2;
             if (op == GREATER_EQUAL) res = d1 >= d2;
             if (op == LESS) res = d1 < d2;
             if (op == LESS_EQUAL) res = d1 <= d2;
             lhs = make_bool(res);
        }
    }
    exit_node(parser, "Comparison");
    return lhs;
}

static Value equality(Parser* parser) {
    enter_node(parser, "Equality");
    Value lhs = comparison(parser);
    while (check(parser, NOT_EQUAL) || check(parser, EQUAL_EQUAL)) {
        TokenType op = parser->current_token.type;
        advance(parser);
        Value rhs = comparison(parser);
        if (parser->executing) {
             bool res = false;
             if (lhs.type == VAL_INT && rhs.type == VAL_INT) {
                 res = (lhs.as.int_val == rhs.as.int_val);
             } else if (lhs.type == VAL_BOOL && rhs.type == VAL_BOOL) {
                 res = (lhs.as.bool_val == rhs.as.bool_val);
             } else {
                 double d1 = (lhs.type == VAL_INT) ? lhs.as.int_val : lhs.as.double_val;
                 double d2 = (rhs.type == VAL_INT) ? rhs.as.int_val : rhs.as.double_val;
                 res = (d1 == d2);
             }
             if (op == NOT_EQUAL) res = !res;
             lhs = make_bool(res);
        }
    }
    exit_node(parser, "Equality");
    return lhs;
}

static Value logical_and(Parser* parser) {
    enter_node(parser, "LogicalAnd");
    Value lhs = equality(parser);
    while (match(parser, AND_AND)) {
        Value rhs = equality(parser);
        if (parser->executing) {
            bool b1 = (lhs.type == VAL_BOOL) ? lhs.as.bool_val : (lhs.as.int_val != 0);
            bool b2 = (rhs.type == VAL_BOOL) ? rhs.as.bool_val : (rhs.as.int_val != 0);
            lhs = make_bool(b1 && b2);
        }
    }
    exit_node(parser, "LogicalAnd");
    return lhs;
}

static Value logical_or(Parser* parser) {
    enter_node(parser, "LogicalOr");
    Value lhs = logical_and(parser);
    while (match(parser, OR_OR)) {
        Value rhs = logical_and(parser);
        if (parser->executing) {
            bool b1 = (lhs.type == VAL_BOOL) ? lhs.as.bool_val : (lhs.as.int_val != 0);
            bool b2 = (rhs.type == VAL_BOOL) ? rhs.as.bool_val : (rhs.as.int_val != 0);
            lhs = make_bool(b1 || b2);
        }
    }
    exit_node(parser, "LogicalOr");
    return lhs;
}

static Value expression(Parser* parser) {
    enter_node(parser, "Expression");
    Value v = logical_or(parser);
    exit_node(parser, "Expression");
    return v;
}

static void declaration_statement(Parser* parser) {
    enter_node(parser, "DeclarationStatement");
    if (match(parser, TYPE)) {}
    else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) {
        advance(parser);
    }
    consume(parser, IDENTIFIER, "Expect variable name.");
    char name[64];
    strncpy(name, parser->previous_token.lexeme, 63);
    
    Value init = make_int(0);
    if (match(parser, EQUAL)) init = expression(parser);
    consume(parser, SEMICOLON, "Expect ';' after variable declaration.");
    
    if (parser->executing) env_define(&parser->env, name, init, false);
    exit_node(parser, "DeclarationStatement");
}

static void assignment_statement(Parser* parser) {
    enter_node(parser, "AssignmentStatement");
    char name[64];
    strncpy(name, parser->previous_token.lexeme, 63);

    TokenType op = parser->current_token.type;
    advance(parser); // consume =, +=, etc.
    
    Value rhs = expression(parser);
    consume(parser, SEMICOLON, "Expect ';' after assignment.");
    
    if (parser->executing) {
        if (op == EQUAL) {
             env_assign(parser->env, name, rhs);
        } else {
             Value lhs;
             if (env_get(parser->env, name, &lhs)) {
                 if (op == PLUS_EQUAL) lhs = val_add(lhs, rhs);
                 if (op == MINUS_EQUAL) lhs = val_sub(lhs, rhs);
                 if (op == STAR_EQUAL) lhs = val_mul(lhs, rhs);
                 if (op == SLASH_EQUAL) lhs = val_div(lhs, rhs);
                 env_assign(parser->env, name, lhs);
             }
        }
    }
    exit_node(parser, "AssignmentStatement");
}

static void input_statement(Parser* parser) {
    enter_node(parser, "InputStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'input'.");
    consume(parser, IDENTIFIER, "Expect variable name in input.");
    char name[64];
    strncpy(name, parser->previous_token.lexeme, 63);
    consume(parser, RIGHT_PAREN, "Expect ')' after input variable.");
    consume(parser, SEMICOLON, "Expect ';' after input statement.");
    
    if (parser->executing) {
        int val;
        printf("Enter value for %s: ", name);
        if(scanf("%d", &val)) {
            env_assign(parser->env, name, make_int(val));
        }
    }
    exit_node(parser, "InputStatement");
}

static void output_statement(Parser* parser) {
    enter_node(parser, "OutputStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'print'.");
    Value v = expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after print expression.");
    consume(parser, SEMICOLON, "Expect ';' after print statement.");
    
    if (parser->executing) {
        if (v.type == VAL_INT) printf("%d\n", v.as.int_val);
        else if (v.type == VAL_DOUBLE) printf("%f\n", v.as.double_val);
        else if (v.type == VAL_STRING) printf("%s\n", v.as.string_val);
        else if (v.type == VAL_BOOL) printf("%s\n", v.as.bool_val ? "true" : "false");
        else if (v.type == VAL_CHAR) printf("%c\n", v.as.char_val);
        else printf("null\n");
    }
    exit_node(parser, "OutputStatement");
}

static void jump_to(Parser* parser, int token_index) {
    parser->current_index = token_index;
    if (parser->current_index < parser->token_list->count)
        parser->next_token = parser->token_list->tokens[parser->current_index++];
    else
        parser->next_token = create_token(TOKEN_EOF, "", "", 0, 0);
    advance(parser);
}

static void while_statement(Parser* parser) {
    enter_node(parser, "WhileStatement");
    int loop_start = parser->current_index - 2; 
    
    if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "its") == 0) advance(parser);
    consume(parser, LEFT_PAREN, "Expect '(' after 'while'.");
    Value cond = expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
    
    if (parser->executing) {
        bool b = (cond.type == VAL_BOOL) ? cond.as.bool_val : (cond.as.int_val != 0);
        while (b) {
            statement(parser);
            jump_to(parser, loop_start);
            parser->trace_parse = false;
            
            // Re-eval condition
            if (check(parser, NOISE_WORD)) advance(parser);
            consume(parser, LEFT_PAREN, "msg");
            cond = expression(parser);
            consume(parser, RIGHT_PAREN, "msg");
            b = (cond.type == VAL_BOOL) ? cond.as.bool_val : (cond.as.int_val != 0);
        }
        parser->executing = false;
        statement(parser);
        parser->executing = true;
    } else {
        statement(parser);
    }
    
    parser->trace_parse = true;
    exit_node(parser, "WhileStatement");
}

static void for_statement(Parser* parser) {
    enter_node(parser, "ForStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'for'.");
    
    if (match(parser, SEMICOLON)) {}
    else if (match(parser, TYPE)) declaration_statement(parser);
    else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) {
        advance(parser);
        declaration_statement(parser);
    }
    else if (match(parser, IDENTIFIER)) {
         assignment_statement(parser); 
    }
    else error(parser, "Expect variable declaration or assignment in for loop.");
    
    int cond_loc = parser->current_index - 2;
    /* Debug */
    /* printf("DEBUG: Cond loc: %d. Lexeme: %s\n", cond_loc, parser->token_list->tokens[cond_loc].lexeme); */ 

    bool first_pass = true;
    
    while(1) { 
        if (!first_pass) {
             /* printf("Jumping to cond %d\n", cond_loc); */
             jump_to(parser, cond_loc);
             parser->trace_parse = false;
        }
        
        Value cond = make_bool(true);
        if (!check(parser, SEMICOLON)) cond = expression(parser);
        consume(parser, SEMICOLON, "Expect ';' after loop condition.");
        
        bool b = (cond.type == VAL_BOOL) ? cond.as.bool_val : (cond.as.int_val != 0);
        if (!parser->executing) b = false; 
        
        int inc_loc = parser->current_index - 2;
        
        bool old_exec = parser->executing;
        parser->executing = false;
        if (!check(parser, RIGHT_PAREN)) {
             expression(parser);
        }
        consume(parser, RIGHT_PAREN, "Expect ')' after for clauses.");
        parser->executing = old_exec;
        
        if (b) {
            statement(parser); 
            parser->trace_parse = false;
            jump_to(parser, inc_loc);
            parser->executing = true;
            if (!check(parser, RIGHT_PAREN)) { 
                expression(parser); 
            }
            first_pass = false;
        } else {
             if (parser->executing) { 
                 parser->executing = false;
                 statement(parser);
                 parser->executing = true;
             } else {
                 statement(parser); 
             }
             break;
        }
    }
    parser->trace_parse = true;
    exit_node(parser, "ForStatement");
}

static void foreach_statement(Parser* parser) {
    enter_node(parser, "ForeachStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'foreach'.");
    if (match(parser, TYPE)) {}
    else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) advance(parser);
    else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "var") == 0) advance(parser);
    else error(parser, "Expect type or 'var' in foreach.");
    consume(parser, IDENTIFIER, "Expect variable name.");
    if (check(parser, RESERVED_WORD) && strcmp(parser->current_token.lexeme, "in") == 0) {
        advance(parser);
    } else {
        error(parser, "Expect 'in' after variable.");
    }
    expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after collection.");
    statement(parser);
    exit_node(parser, "ForeachStatement");
}

static void switch_statement(Parser* parser) {
    enter_node(parser, "SwitchStatement");
    consume(parser, LEFT_PAREN, "Expect '(' after 'switch'.");
    expression(parser);
    consume(parser, RIGHT_PAREN, "Expect ')' after switch expression.");
    consume(parser, LEFT_BRACE, "Expect '{' before switch cases.");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        if (match(parser, CASE)) {
            enter_node(parser, "CaseClause");
            expression(parser);
            consume(parser, COLON, "Expect ':' after case expression.");
            while (!check(parser, CASE) && !check(parser, DEFAULT) && !check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
                statement(parser);
            }
            exit_node(parser, "CaseClause");
        } else if (match(parser, DEFAULT)) {
            enter_node(parser, "DefaultClause");
            consume(parser, COLON, "Expect ':' after default.");
            while (!check(parser, CASE) && !check(parser, DEFAULT) && !check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
                statement(parser);
            }
            exit_node(parser, "DefaultClause");
        } else {
            error(parser, "Expect 'case' or 'default' inside switch.");
            advance(parser);
        }
    }
    consume(parser, RIGHT_BRACE, "Expect '}' after switch body.");
    exit_node(parser, "SwitchStatement");
}

static void do_while_statement(Parser* parser) {
    enter_node(parser, "DoWhileStatement");
    int loop_start = parser->current_index - 2;
    consume(parser, LEFT_BRACE, "Expect '{' after 'do'.");
    
    bool first = true;
    while(1) {
        if (!first) {
            jump_to(parser, loop_start);
            consume(parser, LEFT_BRACE, "msg"); // Re-consume
            parser->trace_parse = false;
        }
        
        // Execute block
        while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) statement(parser);
        consume(parser, RIGHT_BRACE, "Expect '}' after block.");
        
        if (check(parser, RESERVED_WORD) && strcmp(parser->current_token.lexeme, "while") == 0) {
            advance(parser);
        } else {
            error(parser, "Expect 'while' after do-block.");
        }
        
        consume(parser, LEFT_PAREN, "Expect '(' after 'while'.");
        Value cond = expression(parser);
        consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
        consume(parser, SEMICOLON, "Expect ';' after do-while.");
        
        if (parser->executing) {
            bool b = (cond.type == VAL_BOOL) ? cond.as.bool_val : (cond.as.int_val != 0);
            if (!b) break;
        } else {
            break;
        }
        first = false;
    }
    
    parser->trace_parse = true;
    exit_node(parser, "DoWhileStatement");
}

static void next_statement(Parser* parser) {
    enter_node(parser, "NextStatement");
    consume(parser, SEMICOLON, "Expect ';' after 'next'.");
    exit_node(parser, "NextStatement");
}

static void enum_declaration(Parser* parser) {
    enter_node(parser, "EnumDeclaration");
    consume(parser, IDENTIFIER, "Expect enum name.");
    consume(parser, LEFT_BRACE, "Expect '{' before enum members.");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        consume(parser, IDENTIFIER, "Expect enum member name.");
        if (match(parser, EQUAL)) {
            expression(parser);
        }
        if (match(parser, COMMA)) {} 
        else break;
    }
    consume(parser, RIGHT_BRACE, "Expect '}' after enum members.");
    exit_node(parser, "EnumDeclaration");
}

static void struct_declaration(Parser* parser) {
    enter_node(parser, "StructDefinition");
    consume(parser, IDENTIFIER, "Expect struct name.");
    consume(parser, LEFT_BRACE, "Expect '{' before struct members.");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        if (match(parser, TYPE)) {}
        else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) advance(parser);
        else error(parser, "Expect type in struct member.");
        
        consume(parser, IDENTIFIER, "Expect member name.");
        consume(parser, SEMICOLON, "Expect ';' after member.");
    }
    consume(parser, RIGHT_BRACE, "Expect '}' after struct members.");
    exit_node(parser, "StructDefinition");
}

static void record_declaration(Parser* parser) {
    enter_node(parser, "RecordDeclaration");
    consume(parser, IDENTIFIER, "Expect record name.");
    consume(parser, LEFT_BRACE, "Expect '{' before record members.");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        if (match(parser, REQ)) {}
        
        if (match(parser, TYPE)) {}
        else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) advance(parser);
        else error(parser, "Expect type in record member.");
        
        consume(parser, IDENTIFIER, "Expect member name.");
        
        if (match(parser, EQUAL)) {
            expression(parser);
        }
        
        consume(parser, SEMICOLON, "Expect ';' after member.");
    }
    consume(parser, RIGHT_BRACE, "Expect '}' after record members.");
    exit_node(parser, "RecordDeclaration");
}

static void class_declaration(Parser* parser) {
    enter_node(parser, "ClassDeclaration");
    consume(parser, IDENTIFIER, "Expect class name.");
    consume(parser, LEFT_BRACE, "Expect '{' before class body.");
    while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        if (match(parser, PUB) || match(parser, PRIV) || match(parser, PROT)) {}
        
        // Handle optional 'rdo'
        if (check(parser, RESERVED_WORD) && strcmp(parser->current_token.lexeme, "rdo") == 0) {
            advance(parser);
        }

        if (match(parser, TYPE)) {}
        else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) advance(parser);
        else error(parser, "Expect type or void in class member.");
        
        consume(parser, IDENTIFIER, "Expect member name.");
        
        if (match(parser, LEFT_PAREN)) {
            enter_node(parser, "MethodDeclaration");
            if (!check(parser, RIGHT_PAREN)) {
                do {
                    if (match(parser, TYPE)) {}
                    else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) advance(parser);
                    
                    // Allow IDENTIFIER or KEYWORD (contextual) as argument name
                    if (check(parser, IDENTIFIER) || check(parser, KEYWORD)) {
                        advance(parser);
                    } else {
                        error(parser, "Expect argument name.");
                    }
                } while (match(parser, COMMA));
            }
            consume(parser, RIGHT_PAREN, "Expect ')' after arguments.");
            consume(parser, LEFT_BRACE, "Expect '{' before method body.");
            block(parser);
            exit_node(parser, "MethodDeclaration");
        } else if (match(parser, LEFT_BRACE)) {
            enter_node(parser, "PropertyDeclaration");
            while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
                if (match(parser, GET) || match(parser, SET) || match(parser, INIT)) {
                    // Accessor body: { ... } or ;
                    if (match(parser, LEFT_BRACE)) {
                        while (!check(parser, RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
                            statement(parser);
                        }
                        consume(parser, RIGHT_BRACE, "Expect '}' after accessor body.");
                    } else {
                        consume(parser, SEMICOLON, "Expect ';' after accessor (auto-implemented).");
                    }
                } else {
                    error(parser, "Expect get, set, or init.");
                    advance(parser);
                }
            }
            consume(parser, RIGHT_BRACE, "Expect '}' after property body.");
            exit_node(parser, "PropertyDeclaration");
        } else {
            // Field Declaration
            if (match(parser, EQUAL)) {
                expression(parser);
            }
            consume(parser, SEMICOLON, "Expect ';' after field.");
        }
    }
    consume(parser, RIGHT_BRACE, "Expect '}' after class body.");
    exit_node(parser, "ClassDeclaration");
}

static void statement(Parser* parser) {
    enter_node(parser, "Statement");
    if (match(parser, PLUS_PLUS) || match(parser, MINUS_MINUS)) {
        enter_node(parser, "IncrementStatement");
        TokenType op = parser->previous_token.type;
        consume(parser, IDENTIFIER, "Expect identifier after prefix operator.");
        char name[64];
        strncpy(name, parser->previous_token.lexeme, 63);
        
        if (parser->executing) {
            Value v;
            if (env_get(parser->env, name, &v)) {
                if(op == PLUS_PLUS) v = val_add(v, make_int(1));
                else v = val_sub(v, make_int(1));
                env_assign(parser->env, name, v);
            }
        }
        consume(parser, SEMICOLON, "Expect ';' after increment/decrement.");
        exit_node(parser, "IncrementStatement");
    }
    else if (match(parser, TYPE)) declaration_statement(parser);
    else if (check(parser, KEYWORD) && strcmp(parser->current_token.lexeme, "str") == 0) {
        advance(parser);
        declaration_statement(parser);
    }
    else if (match(parser, SWITCH)) switch_statement(parser);
    else if (match(parser, DO)) do_while_statement(parser);
    else if (match(parser, NEXT)) next_statement(parser);
    else if (match(parser, BREAK)) { consume(parser, SEMICOLON, "Expect ';' after break."); }
    else if (match(parser, CLASS)) class_declaration(parser);
    else if (match(parser, STRUCT)) struct_declaration(parser);
    else if (match(parser, ENUM)) enum_declaration(parser);
    else if (match(parser, RECORD)) record_declaration(parser);
    else if ((check(parser, PUB) || check(parser, PRIV)) && parser->next_token.type == RECORD) {
        advance(parser); 
        advance(parser); 
        record_declaration(parser);
    }
    else if (check(parser, RESERVED_WORD) || check(parser, KEYWORD)) {
        if (strcmp(parser->current_token.lexeme, "while") == 0) { advance(parser); while_statement(parser); }
        else if (strcmp(parser->current_token.lexeme, "for") == 0) { advance(parser); for_statement(parser); }
        else if (strcmp(parser->current_token.lexeme, "foreach") == 0) { advance(parser); foreach_statement(parser); }
        else if (strcmp(parser->current_token.lexeme, "if") == 0) {
            enter_node(parser, "IfStatement");
            advance(parser);
            if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "at") == 0) advance(parser);
            consume(parser, LEFT_PAREN, "Expect '(' after 'if'.");
            Value cond = expression(parser);
            consume(parser, RIGHT_PAREN, "Expect ')' after condition.");
            if (check(parser, NOISE_WORD) && strcmp(parser->current_token.lexeme, "then") == 0) advance(parser);
            
            bool parent_exec = parser->executing;
            bool b = (cond.type == VAL_BOOL) ? cond.as.bool_val : (cond.as.int_val != 0);
            
            parser->executing = parent_exec && b;
            statement(parser);
            parser->executing = parent_exec; 
            
            if (check(parser, RESERVED_WORD) && strcmp(parser->current_token.lexeme, "else") == 0) {
                advance(parser);
                parser->executing = parent_exec && !b;
                statement(parser);
                parser->executing = parent_exec;
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
        } else if (strcmp(parser->current_token.lexeme, "let") == 0) {
            enter_node(parser, "LetStatement");
            advance(parser);
            consume(parser, IDENTIFIER, "Expect variable name after 'let'.");
            char name[64];
            strncpy(name, parser->previous_token.lexeme, 63);
            consume(parser, EQUAL, "Expect '=' after variable name.");
            Value init = expression(parser);
            consume(parser, SEMICOLON, "Expect ';' after let statement.");
            if (parser->executing) env_define(&parser->env, name, init, false);
            exit_node(parser, "LetStatement");
        } else if (strcmp(parser->current_token.lexeme, "set") == 0) {
            enter_node(parser, "SetStatement");
            advance(parser);
            consume(parser, IDENTIFIER, "Expect variable name after 'set'.");
            char name[64];
            strncpy(name, parser->previous_token.lexeme, 63);
            consume(parser, EQUAL, "Expect '=' after variable name.");
            Value rhs = expression(parser);
            consume(parser, SEMICOLON, "Expect ';' after set statement.");
            if (parser->executing) env_assign(parser->env, name, rhs);
            exit_node(parser, "SetStatement");
        } else if (strcmp(parser->current_token.lexeme, "var") == 0 || 
                   strcmp(parser->current_token.lexeme, "const") == 0 ||
                   strcmp(parser->current_token.lexeme, "dyn") == 0) {
            advance(parser);
            declaration_statement(parser);
        } else {
            error(parser, "Unexpected keyword at start of statement.");
            advance(parser);
        }
    } else if (match(parser, LEFT_BRACE)) {
        block(parser);
    } else if (match(parser, IDENTIFIER)) {
        char name[64];
        strncpy(name, parser->previous_token.lexeme, 63);
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
            TokenType op = parser->current_token.type;
            advance(parser);
            if (parser->executing) {
                Value v;
                if(env_get(parser->env, name, &v)) {
                     Value new_val = (op == PLUS_PLUS) ? val_add(v, make_int(1)) : val_sub(v, make_int(1));
                     env_assign(parser->env, name, new_val);
                }
            }
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

    const char* suffix = ".cytho";
    size_t input_len = strlen(argv[1]);
    size_t suffix_len = strlen(suffix);
    
    // 1. File Extension Check
    if (input_len < suffix_len || strcmp(argv[1] + input_len - suffix_len, suffix) != 0) {
        fprintf(stderr, "Error: Invalid file type. Expected '.cytho' extension.\n");
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

    // 2. Lexical Analysis -> Generate Symbol Table File
    char symbol_table_path[256];
    strncpy(symbol_table_path, argv[1], input_len);
    symbol_table_path[input_len] = '\0';
    strcat(symbol_table_path, ".symboltable.txt");
    
    Lexer* lexer_for_table = lexer_create(source);
    write_symbol_table(lexer_for_table, symbol_table_path);
    printf("Lexical Analysis Complete. Symbol table written to: %s\n", symbol_table_path);
    
    // Check if symbol table valid
    // ...

    // Cleanup Lexer
    free(lexer_for_table->trie);
    free(lexer_for_table);
    free(source); // Source no longer needed after Lexing phase

    // 3. Syntax Analysis -> Read Token Stream from Symbol Table
    // Requirement: "Input: must be read one by one from the symbol table"
    TokenList tokens = read_tokens_from_symbol_table(symbol_table_path);
    if (tokens.count == 0 && tokens.tokens == NULL) {
        fprintf(stderr, "Error: Failed to read tokens from symbol table or empty file.\n");
        return 1;
    }
    printf("Read %d tokens from symbol table.\n", tokens.count);

    // 4. Generate Parse Tree
    char parse_tree_path[256];
    strncpy(parse_tree_path, argv[1], input_len);
    parse_tree_path[input_len] = '\0';
    strcat(parse_tree_path, ".parsetree.txt");
    
    FILE* output_file = fopen(parse_tree_path, "w");
    if (!output_file) fprintf(stderr, "Error: Cannot create output file '%s'\n", parse_tree_path);
    else printf("Writing parse tree to: %s\n", parse_tree_path);

    // Run Parser with Token List
    Parser* parser = parser_create(&tokens);
    parser->output_file = output_file;
    
    parser_parse(parser);

    // Cleanup
    if (output_file) fclose(output_file);
    free(parser);

    return 0;
}
