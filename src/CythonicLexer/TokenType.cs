namespace CythonicLexer;

public enum TokenType
{
    // Keywords and Types
    KEYWORD,           // Contextual keywords (21 total)
    RESERVED_WORD,     // Reserved words (33 total)
    TYPE,
    IDENTIFIER,
    BOOLEAN_LITERAL,
    NOISE_WORD,
    
    // Literals
    NUMBER,
    STRING_LITERAL,
    CHAR_LITERAL,
    
    // Arithmetic Operators
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    PLUS_PLUS,      // ++
    MINUS_MINUS,    // --
    
    // Assignment
    EQUAL,          // =
    
    // Comparison Operators
    EQUAL_EQUAL,    // ==
    NOT_EQUAL,      // !=
    GREATER,        // >
    LESS,           // <
    GREATER_EQUAL,  // >=
    LESS_EQUAL,     // <=
    
    // Logical Operators
    AND_AND,        // &&
    OR_OR,          // ||
    NOT,            // !
    
    // Bitwise Operators
    AND,            // &
    OR,             // |
    XOR,            // ^
    TILDE,          // ~
    
    // Delimiters
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    SEMICOLON,      // ;
    COMMA,          // ,
    DOT,            // .
    COLON,          // :
    QUESTION,       // ?
    
    // Other
    COMMENT,
    EOF
}
