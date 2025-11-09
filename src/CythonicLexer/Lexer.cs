using System.Text;

namespace CythonicLexer;

/// <summary>
/// Production lexer for the Cythonic language. Implements an explicit DFA over the input stream.
/// </summary>
public sealed class Lexer
{
    private readonly string _source;
    private readonly KeywordTrie _keywords = new();
    private readonly List<Token> _tokens = new();
    private int _index;
    private int _line = 1;
    private int _column = 1;
    private int _columnBias;
    private int _lineBias;

    public Lexer(string source)
    {
        _source = source ?? throw new ArgumentNullException(nameof(source));
    }

    public IReadOnlyList<Token> Lex()
    {
        while (!IsAtEnd())
        {
            SkipWhitespaceAndComments();
            if (IsAtEnd())
            {
                break;
            }

            var startLine = _line;
            var startColumn = _column;
            var startIndex = _index;
            var startLineBias = _lineBias;
            var startColumnBias = _columnBias;
            var current = Peek();

            if (IsIdentifierStart(current))
            {
                LexIdentifierOrKeyword(startIndex, startLine, startColumn, startLineBias, startColumnBias);
                continue;
            }

            if (IsDigit(current))
            {
                LexNumber(startIndex, startLine, startColumn, startLineBias, startColumnBias, startsWithDot: false);
                continue;
            }

            if (current == '.' && IsDigit(Peek(1)))
            {
                LexNumber(startIndex, startLine, startColumn, startLineBias, startColumnBias, startsWithDot: true);
                continue;
            }

            if (current == '\'')
            {
                LexCharLiteral(startIndex, startLine, startColumn, startLineBias, startColumnBias);
                continue;
            }

            if (current == '"')
            {
                LexStringLiteral(startIndex, startLine, startColumn, startLineBias, startColumnBias);
                continue;
            }

            if (IsOperatorOrDelimiterStart(current))
            {
                LexOperatorOrDelimiter(startIndex, startLine, startColumn, startLineBias, startColumnBias);
                continue;
            }

            throw new LexerException($"Invalid character '{current}'", _line, _column);
        }

        var eofLine = AdjustLine(_line, _lineBias);
        var eofColumn = AdjustColumn(_column, _columnBias);
        _tokens.Add(new Token(TokenType.EOF, string.Empty, eofLine, eofColumn, string.Empty));
        return _tokens;
    }

    private void SkipWhitespaceAndComments()
    {
        while (!IsAtEnd())
        {
            var c = Peek();
            if (c == ' ' || c == '\t')
            {
                Advance();
                continue;
            }

            if (c == '\r' || c == '\n')
            {
                Advance();
                continue;
            }

            if (c == '/' && Peek(1) == '/')
            {
                Advance();
                Advance();
                while (!IsAtEnd())
                {
                    var next = Peek();
                    if (next == '\r' || next == '\n')
                    {
                        break;
                    }

                    Advance();
                }

                continue;
            }

            if (c == '/' && Peek(1) == '*')
            {
                Advance();
                Advance();
                var terminated = false;
                while (!IsAtEnd())
                {
                    var next = Advance();
                    if (next == '\n')
                    {
                        _lineBias++;
                    }
                    if (next == '*' && Peek() == '/')
                    {
                        Advance();
                        terminated = true;
                        break;
                    }
                }

                if (!terminated)
                {
                    throw new LexerException("Unterminated multi-line comment", _line, _column);
                }

                continue;
            }

            break;
        }
    }

    private void LexIdentifierOrKeyword(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias)
    {
        var normalized = new StringBuilder();
        var keywordState = 0;
        var keywordPossible = true;
        var onlyLetters = true;
        var lastAcceptingType = TokenType.IDENTIFIER;
        var hasAcceptingKeyword = false;

        while (!IsAtEnd())
        {
            var c = Peek();
            if (!IsIdentifierPart(c))
            {
                break;
            }

            var lower = AsciiLower(c);
            if (normalized.Length < 31)
            {
                normalized.Append(lower);
            }

            if (!IsLetter(c))
            {
                onlyLetters = false;
            }

            if (!IsLetter(c))
            {
                keywordPossible = false;
            }
            else if (keywordPossible)
            {
                var nextState = _keywords.Move(keywordState, lower);
                if (nextState == -1)
                {
                    keywordPossible = false;
                }
                else
                {
                    keywordState = nextState;
                    if (_keywords.TryGetAcceptingType(keywordState, out var keywordType))
                    {
                        lastAcceptingType = keywordType;
                        hasAcceptingKeyword = true;
                    }
                }
            }

            Advance();
        }

        var raw = ExtractRaw(startIndex);
        var lexeme = normalized.ToString();
        TokenType type;

        if (keywordPossible && onlyLetters && hasAcceptingKeyword)
        {
            type = lastAcceptingType;
        }
        else
        {
            type = TokenType.IDENTIFIER;
        }

        var tokenLine = AdjustLine(startLine, startLineBias);
        var tokenColumn = AdjustColumn(startColumn, startColumnBias);
        _tokens.Add(new Token(type, lexeme, tokenLine, tokenColumn, raw));
    }

    private void LexNumber(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias, bool startsWithDot)
    {
        var normalized = new StringBuilder();
        var hasIntegerDigits = false;
        var hasFractionDigits = false;
        var hasDot = false;
        var hasExponent = false;
        var exponentDigits = false;

        if (startsWithDot)
        {
            hasDot = true;
            AppendNumericChar(normalized, Advance());
            while (IsDigit(Peek()))
            {
                hasFractionDigits = true;
                AppendNumericChar(normalized, Advance());
            }

            if (!hasFractionDigits)
            {
                throw new LexerException("Invalid floating literal", _line, _column);
            }
        }
        else
        {
            while (IsDigit(Peek()))
            {
                hasIntegerDigits = true;
                AppendNumericChar(normalized, Advance());
            }

            if (Peek() == '.' && IsDigit(Peek(1)))
            {
                hasDot = true;
                AppendNumericChar(normalized, Advance());
                while (IsDigit(Peek()))
                {
                    hasFractionDigits = true;
                    AppendNumericChar(normalized, Advance());
                }
            }
            else if (Peek() == '.')
            {
                hasDot = true;
                AppendNumericChar(normalized, Advance());
            }
        }

        var peek = Peek();
        if (peek == 'e' || peek == 'E')
        {
            hasExponent = true;
            AppendNumericChar(normalized, AsciiLower(Advance()));
            var sign = Peek();
            if (sign == '+' || sign == '-')
            {
                AppendNumericChar(normalized, Advance());
            }

            while (IsDigit(Peek()))
            {
                exponentDigits = true;
                AppendNumericChar(normalized, Advance());
            }

            if (!exponentDigits)
            {
                throw new LexerException("Invalid exponent in numeric literal", _line, _column);
            }
        }

        if (!hasIntegerDigits && !hasFractionDigits)
        {
            throw new LexerException("Invalid numeric literal", startLine, startColumn);
        }

        if (hasExponent && !hasDot && !hasIntegerDigits)
        {
            throw new LexerException("Invalid numeric literal", startLine, startColumn);
        }

        var trailing = Peek();
        if (IsLetter(trailing) || trailing == '_')
        {
            throw new LexerException("Invalid numeric literal", _line, _column);
        }

        var raw = ExtractRaw(startIndex);
        var lexeme = normalized.ToString();
        var tokenLine = AdjustLine(startLine, startLineBias);
        var tokenColumn = AdjustColumn(startColumn, startColumnBias);
        _tokens.Add(new Token(TokenType.NUMBER, lexeme, tokenLine, tokenColumn, raw));
    }

    private void LexCharLiteral(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias)
    {
        Advance(); // opening quote
        if (IsAtEnd())
        {
            throw new LexerException("Unterminated char literal", startLine, startColumn);
        }

        char value;
        var c = Advance();
        if (c == '\\')
        {
            value = ReadEscape(startLine, startColumn);
        }
        else
        {
            if (c == '\'' || c == '\r' || c == '\n')
            {
                throw new LexerException("Invalid char literal", _line, _column);
            }

            value = c;
        }

        if (IsAtEnd() || Peek() != '\'')
        {
            throw new LexerException("Unterminated char literal", _line, _column);
        }

        Advance(); // closing quote
        var raw = ExtractRaw(startIndex);
        var lexeme = new string(value, 1);
        var tokenLine = AdjustLine(startLine, startLineBias);
        var tokenColumn = AdjustColumn(startColumn, startColumnBias);
        _tokens.Add(new Token(TokenType.CHAR_LITERAL, lexeme, tokenLine, tokenColumn, raw));
    }

    private void LexStringLiteral(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias)
    {
        Advance(); // opening quote
        var builder = new StringBuilder();

        while (!IsAtEnd())
        {
            var c = Advance();
            if (c == '"')
            {
                var raw = ExtractRaw(startIndex);
                var lexeme = builder.ToString();
                var tokenLine = AdjustLine(startLine, startLineBias);
                var tokenColumn = AdjustColumn(startColumn, startColumnBias);
                _tokens.Add(new Token(TokenType.STRING_LITERAL, lexeme, tokenLine, tokenColumn, raw));
                return;
            }

            if (c == '\\')
            {
                builder.Append(ReadEscape(startLine, startColumn));
                continue;
            }

            if (c == '\r' || c == '\n')
            {
                throw new LexerException("Unterminated string literal", _line, _column);
            }

            builder.Append(c);
        }

        throw new LexerException("Unterminated string literal", startLine, startColumn);
    }

    private void LexOperatorOrDelimiter(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias)
    {
        var first = Advance();
        TokenType type;
        string lexeme;

        switch (first)
        {
            case '+':
                if (Peek() == '+')
                {
                    lexeme = "++";
                    Advance();
                }
                else
                {
                    lexeme = "+";
                }

                type = TokenType.OPERATOR;
                break;
            case '-':
                if (Peek() == '-')
                {
                    lexeme = "--";
                    Advance();
                }
                else
                {
                    lexeme = "-";
                }

                type = TokenType.OPERATOR;
                break;
            case '=':
                if (Peek() == '=')
                {
                    lexeme = "==";
                    Advance();
                }
                else
                {
                    lexeme = "=";
                }

                type = TokenType.OPERATOR;
                break;
            case '!':
                if (Peek() == '=')
                {
                    lexeme = "!=";
                    Advance();
                }
                else
                {
                    lexeme = "!";
                }

                type = TokenType.OPERATOR;
                break;
            case '>':
                if (Peek() == '=')
                {
                    lexeme = ">=";
                    Advance();
                }
                else
                {
                    lexeme = ">";
                }

                type = TokenType.OPERATOR;
                break;
            case '<':
                if (Peek() == '=')
                {
                    lexeme = "<=";
                    Advance();
                }
                else
                {
                    lexeme = "<";
                }

                type = TokenType.OPERATOR;
                break;
            case '&':
                if (Peek() == '&')
                {
                    lexeme = "&&";
                    Advance();
                }
                else
                {
                    lexeme = "&";
                }

                type = TokenType.OPERATOR;
                break;
            case '|':
                if (Peek() == '|')
                {
                    lexeme = "||";
                    Advance();
                }
                else
                {
                    lexeme = "|";
                }

                type = TokenType.OPERATOR;
                break;
            case '*':
            case '/':
            case '%':
            case '^':
            case '~':
                lexeme = new string(first, 1);
                type = TokenType.OPERATOR;
                break;
            case ':':
            case ';':
            case ',':
            case '.':
            case '(': 
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case '?':
                lexeme = new string(first, 1);
                type = TokenType.DELIMITER;
                break;
            default:
                throw new LexerException($"Unexpected operator character '{first}'", startLine, startColumn);
        }

        var raw = ExtractRaw(startIndex);
        var tokenLine = AdjustLine(startLine, startLineBias);
        var tokenColumn = AdjustColumn(startColumn, startColumnBias);
        _tokens.Add(new Token(type, lexeme, tokenLine, tokenColumn, raw));
    }

    private char ReadEscape(int tokenLine, int tokenColumn)
    {
        if (IsAtEnd())
        {
            throw new LexerException("Unterminated escape sequence", tokenLine, tokenColumn);
        }

        var escape = Advance();
        var value = escape switch
        {
            '\\' => '\\',
            '\"' => '"',
            '\'' => '\'',
            'n' => '\n',
            't' => '\t',
            'r' => '\r',
            'b' => '\b',
            'f' => '\f',
            '0' => '\0',
            _ => throw new LexerException($"Invalid escape character '{escape}'", _line, _column)
        };

        _columnBias++;
        return value;
    }

    private static bool IsOperatorOrDelimiterStart(char c)
    {
        return c switch
        {
            '+' or '-' or '*' or '/' or '%' or '=' or '<' or '>' or '!' or '&' or '|' or '^' or '~' or ':' or ';' or ',' or '.' or '(' or ')' or '{' or '}' or '[' or ']' or '?' => true,
            _ => false
        };
    }

    private static void AppendNumericChar(StringBuilder builder, char c)
    {
        builder.Append(c);
    }

    private char Peek(int lookahead = 0)
    {
        var target = _index + lookahead;
        if (target >= _source.Length)
        {
            return '\0';
        }

        return _source[target];
    }

    private bool IsAtEnd()
    {
        return _index >= _source.Length;
    }

    private char Advance()
    {
        var c = _source[_index++];
        if (c == '\r')
        {
            if (!IsAtEnd() && _source[_index] == '\n')
            {
                _index++;
                c = '\n';
            }

            _line++;
            _column = 1;
            _columnBias = 0;
            return c;
        }

        if (c == '\n')
        {
            _line++;
            _column = 1;
            _columnBias = 0;
            return c;
        }

        _column++;
        return c;
    }

    private string ExtractRaw(int startIndex)
    {
        var length = _index - startIndex;
        return _source.Substring(startIndex, length);
    }

    private static bool IsIdentifierStart(char c)
    {
        return IsLetter(c) || c == '_';
    }

    private static bool IsIdentifierPart(char c)
    {
        return IsLetter(c) || IsDigit(c) || c == '_';
    }

    private static bool IsLetter(char c)
    {
        if (c >= 'A' && c <= 'Z')
        {
            return true;
        }

        if (c >= 'a' && c <= 'z')
        {
            return true;
        }

        return false;
    }

    private static bool IsDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    private static char AsciiLower(char c)
    {
        if (c >= 'A' && c <= 'Z')
        {
            return (char)(c | 0x20);
        }

        return c;
    }

    private static int AdjustLine(int line, int lineBias)
    {
        var adjusted = line - lineBias;
        return adjusted < 1 ? 1 : adjusted;
    }

    private static int AdjustColumn(int column, int columnBias)
    {
        var adjusted = column - columnBias;
        return adjusted < 1 ? 1 : adjusted;
    }
}
