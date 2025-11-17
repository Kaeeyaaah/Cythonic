using System.Collections.Generic;
using System.Linq;
using CythonicLexer;
using Xunit;

namespace CythonicLexer.Tests;

public sealed class LexerTests
{
    [Fact]
    public void Lex_KeywordsAreCaseInsensitive()
    {
        var tokens = LexWithoutEof("If iF IF\n");
        Assert.Equal(3, tokens.Count);
        foreach (var token in tokens)
        {
            Assert.Equal(TokenType.RESERVED_WORD, token.Type);
            Assert.Equal("if", token.Lexeme);
        }

        Assert.Equal(new[] { "If", "iF", "IF" }, tokens.Select(t => t.Raw).ToArray());
    }

    [Fact]
    public void Lex_IdentifiersNormalizeToLowercaseAndLimitLength()
    {
        var longIdentifier = "IdentifierNameThatExceedsThirtyOneCharactersXYZ";
        var source = $"_Id id1 {longIdentifier}\n";
        var tokens = LexWithoutEof(source);

        Assert.Equal(3, tokens.Count);
        Assert.Equal(TokenType.IDENTIFIER, tokens[0].Type);
        Assert.Equal("_id".ToLowerInvariant(), tokens[0].Lexeme);

        Assert.Equal(TokenType.IDENTIFIER, tokens[1].Type);
        Assert.Equal("id1", tokens[1].Lexeme);

        Assert.Equal(TokenType.IDENTIFIER, tokens[2].Type);
        Assert.Equal(longIdentifier.ToLowerInvariant().Substring(0, 31), tokens[2].Lexeme);
    }

    [Fact]
    public void Lex_OperatorsFollowLongestMatch()
    {
        const string source = ">= > ++ + -- - == = != ! && & || |\n";
        var tokens = LexWithoutEof(source);

        var expected = new (TokenType Type, string Lexeme)[]
        {
            (TokenType.GREATER_EQUAL, ">="),
            (TokenType.GREATER, ">"),
            (TokenType.PLUS_PLUS, "++"),
            (TokenType.PLUS, "+"),
            (TokenType.MINUS_MINUS, "--"),
            (TokenType.MINUS, "-"),
            (TokenType.EQUAL_EQUAL, "=="),
            (TokenType.EQUAL, "="),
            (TokenType.NOT_EQUAL, "!="),
            (TokenType.NOT, "!"),
            (TokenType.AND_AND, "&&"),
            (TokenType.AND, "&"),
            (TokenType.OR_OR, "||"),
            (TokenType.OR, "|")
        };

        Assert.Equal(expected.Length, tokens.Count);
        for (var i = 0; i < expected.Length; i++)
        {
            Assert.Equal(expected[i].Type, tokens[i].Type);
            Assert.Equal(expected[i].Lexeme, tokens[i].Lexeme);
        }
    }

    [Fact]
    public void Lex_NumberVariants()
    {
        const string source = "123 0.5 10. .5 1e10 1.23E-4\n";
        var tokens = LexWithoutEof(source);

        var expected = new[] { "123", "0.5", "10.", ".5", "1e10", "1.23e-4" };
        Assert.Equal(expected.Length, tokens.Count);
        for (var i = 0; i < expected.Length; i++)
        {
            Assert.Equal(TokenType.NUMBER, tokens[i].Type);
            Assert.Equal(expected[i], tokens[i].Lexeme);
        }
    }

    [Fact]
    public void Lex_NumberFollowedByIdentifierErrors()
    {
        var ex = Assert.Throws<LexerException>(() => new Lexer("1id").Lex());
        Assert.Contains("Invalid numeric literal", ex.Message);
    }

    [Fact]
    public void Lex_StringAndCharEscapes()
    {
        const string source = "\"Line\\n\" '\\t' '\\''\n";
        var tokens = LexWithoutEof(source);

        Assert.Equal(TokenType.STRING_LITERAL, tokens[0].Type);
        Assert.Equal("Line\n", tokens[0].Lexeme);
        Assert.Equal("\"Line\\n\"", tokens[0].Raw);

        Assert.Equal(TokenType.CHAR_LITERAL, tokens[1].Type);
        Assert.Equal("\t", tokens[1].Lexeme);
        Assert.Equal("'\\t'", tokens[1].Raw);

        Assert.Equal(TokenType.CHAR_LITERAL, tokens[2].Type);
        Assert.Equal("'", tokens[2].Lexeme);
        Assert.Equal("'\\''", tokens[2].Raw);
    }

    [Fact]
    public void Lex_UnterminatedStringErrors()
    {
        var ex = Assert.Throws<LexerException>(() => new Lexer("\"unterminated").Lex());
        Assert.Contains("Unterminated string literal", ex.Message);
    }

    [Fact]
    public void Lex_UnterminatedCharErrors()
    {
        var ex = Assert.Throws<LexerException>(() => new Lexer("'a").Lex());
        Assert.Contains("Unterminated char literal", ex.Message);
    }

    [Fact]
    public void Lex_CommentsAreTokenized()
    {
        const string source = "pub // comment\nclass /* block */ var\n";
        var tokens = LexWithoutEof(source);

        Assert.Collection(tokens,
            t => Assert.Equal(TokenType.RESERVED_WORD, t.Type),
            t => Assert.Equal(TokenType.COMMENT, t.Type),
            t => Assert.Equal(TokenType.RESERVED_WORD, t.Type),
            t => Assert.Equal(TokenType.COMMENT, t.Type),
            t => Assert.Equal(TokenType.KEYWORD, t.Type));

        Assert.Equal("pub", tokens[0].Lexeme);
        Assert.Equal(" comment", tokens[1].Lexeme);
        Assert.Equal("// comment", tokens[1].Raw);
        Assert.Equal("class", tokens[2].Lexeme);
        Assert.Equal(" block ", tokens[3].Lexeme);
        Assert.Equal("/* block */", tokens[3].Raw);
        Assert.Equal("var", tokens[4].Lexeme);
    }

    [Fact]
    public void Lex_UnterminatedCommentErrors()
    {
        var ex = Assert.Throws<LexerException>(() => new Lexer("/* comment").Lex());
        Assert.Contains("Unterminated multi-line comment", ex.Message);
    }

    [Fact]
    public void Lex_SampleProgramMatchesExpectedTokens()
    {
        const string sample = "pub class MyClass {\n" +
                               "    num COUNT = 10;\n" +
                               "    str name = \"Earl\\n\";\n" +
                               "    // increment\n" +
                               "    COUNT++;\n" +
                               "    if (COUNT >= 11) {\n" +
                               "        print(\"ok\");\n" +
                               "    }\n" +
                               "    /* multi\n" +
                               "line comment */\n" +
                               "}\n";

        var tokens = new Lexer(sample).Lex().Where(t => t.Type != TokenType.EOF).ToArray();

        var expected = new[]
        {
            new TokenExpectation(TokenType.RESERVED_WORD, "pub", 1, 1, "pub"),
            new TokenExpectation(TokenType.RESERVED_WORD, "class", 1, 5, "class"),
            new TokenExpectation(TokenType.IDENTIFIER, "myclass", 1, 11, "MyClass"),
            new TokenExpectation(TokenType.LEFT_BRACE, "{", 1, 19, "{"),
            new TokenExpectation(TokenType.TYPE, "num", 2, 5, "num"),
            new TokenExpectation(TokenType.IDENTIFIER, "count", 2, 9, "COUNT"),
            new TokenExpectation(TokenType.EQUAL, "=", 2, 15, "="),
            new TokenExpectation(TokenType.NUMBER, "10", 2, 17, "10"),
            new TokenExpectation(TokenType.SEMICOLON, ";", 2, 19, ";"),
            new TokenExpectation(TokenType.TYPE, "str", 3, 5, "str"),
            new TokenExpectation(TokenType.IDENTIFIER, "name", 3, 9, "name"),
            new TokenExpectation(TokenType.EQUAL, "=", 3, 14, "="),
            new TokenExpectation(TokenType.STRING_LITERAL, "Earl\n", 3, 16, "\"Earl\\n\""),
            new TokenExpectation(TokenType.SEMICOLON, ";", 3, 23, ";"),
            new TokenExpectation(TokenType.COMMENT, " increment", 4, 5, "// increment"),
            new TokenExpectation(TokenType.IDENTIFIER, "count", 5, 5, "COUNT"),
            new TokenExpectation(TokenType.PLUS_PLUS, "++", 5, 10, "++"),
            new TokenExpectation(TokenType.SEMICOLON, ";", 5, 12, ";"),
            new TokenExpectation(TokenType.RESERVED_WORD, "if", 6, 5, "if"),
            new TokenExpectation(TokenType.LEFT_PAREN, "(", 6, 8, "("),
            new TokenExpectation(TokenType.IDENTIFIER, "count", 6, 9, "COUNT"),
            new TokenExpectation(TokenType.GREATER_EQUAL, ">=", 6, 15, ">="),
            new TokenExpectation(TokenType.NUMBER, "11", 6, 18, "11"),
            new TokenExpectation(TokenType.RIGHT_PAREN, ")", 6, 20, ")"),
            new TokenExpectation(TokenType.LEFT_BRACE, "{", 6, 22, "{"),
            new TokenExpectation(TokenType.IDENTIFIER, "print", 7, 9, "print"),
            new TokenExpectation(TokenType.LEFT_PAREN, "(", 7, 14, "("),
            new TokenExpectation(TokenType.STRING_LITERAL, "ok", 7, 15, "\"ok\""),
            new TokenExpectation(TokenType.RIGHT_PAREN, ")", 7, 19, ")"),
            new TokenExpectation(TokenType.SEMICOLON, ";", 7, 20, ";"),
            new TokenExpectation(TokenType.RIGHT_BRACE, "}", 8, 5, "}"),
            new TokenExpectation(TokenType.COMMENT, " multi\nline comment ", 9, 5, "/* multi\nline comment */"),
            new TokenExpectation(TokenType.RIGHT_BRACE, "}", 10, 1, "}")
        };

        Assert.Equal(expected.Length, tokens.Length);
        for (var i = 0; i < expected.Length; i++)
        {
            Assert.Equal(expected[i].Type, tokens[i].Type);
            Assert.Equal(expected[i].Lexeme, tokens[i].Lexeme);
            Assert.Equal(expected[i].Line, tokens[i].Line);
            Assert.Equal(expected[i].Column, tokens[i].Column);
            Assert.Equal(expected[i].Raw, tokens[i].Raw);
        }
    }

    private static List<Token> LexWithoutEof(string source)
    {
        return new Lexer(source).Lex().Where(t => t.Type != TokenType.EOF).ToList();
    }

    private readonly record struct TokenExpectation(TokenType Type, string Lexeme, int Line, int Column, string Raw);
}
