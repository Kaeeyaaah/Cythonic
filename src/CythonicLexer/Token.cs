namespace CythonicLexer;

public sealed record Token(
    TokenType Type,
    string Lexeme,
    int Line,
    int Column,
    string Raw
);
