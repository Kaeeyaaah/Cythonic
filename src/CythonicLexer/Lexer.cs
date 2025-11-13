using System.Text;
using System.IO;
using System.Linq;

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

    public static Lexer FromFile(string filePath)
    {
        if (string.IsNullOrWhiteSpace(filePath))
        {
            throw new ArgumentException("File path cannot be null or empty.", nameof(filePath));
        }

        var fullPath = Path.GetFullPath(filePath);
        
        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException($"Source file not found: {fullPath}", fullPath);
        }

        var extension = Path.GetExtension(fullPath);
        if (!string.Equals(extension, ".cytho", StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException(
                $"Invalid file type. Expected '.cytho' file, but got '{extension}'. " +
                "Cythonic lexer only processes files with the .cytho extension.",
                nameof(filePath));
        }

        var source = File.ReadAllText(fullPath);
        return new Lexer(source);
    }

    public IReadOnlyList<Token> Lex()
    {
        while (!IsAtEnd())
        {
            SkipWhitespace();
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

            // Check for comments first
            if (current == '/' && Peek(1) == '/')
            {
                LexSingleLineComment(startIndex, startLine, startColumn, startLineBias, startColumnBias);
                continue;
            }

            if (current == '/' && Peek(1) == '*')
            {
                LexMultiLineComment(startIndex, startLine, startColumn, startLineBias, startColumnBias);
                continue;
            }

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

    /// <summary>
    /// Writes the symbol table to a text file.
    /// </summary>
    /// <param name="outputPath">The absolute path to the output file.</param>
    public void WriteSymbolTable(string outputPath)
    {
        using var writer = new StreamWriter(outputPath);
        writer.WriteLine("CYTHONIC LEXICAL ANALYZER - SYMBOL TABLE");
        writer.WriteLine("========================================");
        writer.WriteLine();
        writer.WriteLine($"Source file analyzed: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
        writer.WriteLine($"Total tokens: {_tokens.Count(t => t.Type != TokenType.EOF)}");
        writer.WriteLine();
        writer.WriteLine("LINE | COL | TYPE              | LEXEME                        | RAW");
        writer.WriteLine("-----|-----|-------------------|-------------------------------|----------------------------------");

        foreach (var token in _tokens)
        {
            if (token.Type == TokenType.EOF) continue;

            var lexemeDisplay = token.Lexeme.Length > 28
                ? token.Lexeme.Substring(0, 25) + "..."
                : token.Lexeme;

            var rawDisplay = token.Raw.Length > 32
                ? token.Raw.Substring(0, 29) + "..."
                : token.Raw;

            // Escape newlines for display
            lexemeDisplay = lexemeDisplay.Replace("\n", "\\n").Replace("\r", "\\r").Replace("\t", "\\t");
            rawDisplay = rawDisplay.Replace("\n", "\\n").Replace("\r", "\\r").Replace("\t", "\\t");

            writer.WriteLine($"{token.Line,4} | {token.Column,3} | {token.Type,-17} | {lexemeDisplay,-29} | {rawDisplay}");
        }

        writer.WriteLine();
        writer.WriteLine("END OF SYMBOL TABLE");
    }

    private void SkipWhitespace()
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

            break;
        }
    }

    private void LexSingleLineComment(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias)
    {
        // Consume '//'
        Advance();
        Advance();
        
        var contentStart = _index;
        
        while (!IsAtEnd())
        {
            var next = Peek();
            if (next == '\r' || next == '\n')
            {
                break;
            }
            Advance();
        }

        var raw = _source.Substring(startIndex, _index - startIndex);
        var content = _source.Substring(contentStart, _index - contentStart);
        var lexeme = content.ToLowerInvariant();

        var line = AdjustLine(startLine, startLineBias);
        var column = AdjustColumn(startColumn, startColumnBias);

        _tokens.Add(new Token(TokenType.COMMENT, lexeme, line, column, raw));
    }

    private void LexMultiLineComment(int startIndex, int startLine, int startColumn, int startLineBias, int startColumnBias)
    {
        // Consume opening '/*'
        Advance();
        Advance();
        
        var contentStart = _index;
        var depth = 1; // Track nesting depth
        var terminated = false;
        
        while (!IsAtEnd() && depth > 0)
        {
            var current = Peek();
            
            // Check for opening nested comment '/*'
            if (current == '/' && Peek(1) == '*')
            {
                depth++;
                Advance(); // consume '/'
                Advance(); // consume '*'
                continue;
            }
            
            // Check for closing comment '*/'
            if (current == '*' && Peek(1) == '/')
            {
                depth--;
                if (depth == 0)
                {
                    // Capture content before consuming final '*/'
                    var contentEnd = _index;
                    Advance(); // consume '*'
                    Advance(); // consume '/'
                    
                    var raw = _source.Substring(startIndex, _index - startIndex);
                    var content = _source.Substring(contentStart, contentEnd - contentStart);
                    var lexeme = content.ToLowerInvariant();

                    var line = AdjustLine(startLine, startLineBias);
                    var column = AdjustColumn(startColumn, startColumnBias);

                    _tokens.Add(new Token(TokenType.COMMENT, lexeme, line, column, raw));
                    
                    terminated = true;
                    break;
                }
                else
                {
                    // It's a closing tag for a nested comment, just consume it
                    Advance(); // consume '*'
                    Advance(); // consume '/'
                    continue;
                }
            }
            
            // Track newlines for line bias
            var next = Advance();
            if (next == '\n')
            {
                _lineBias++;
            }
        }

        if (!terminated)
        {
            throw new LexerException("Unterminated multi-line comment", _line, _column);
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
                    type = TokenType.PLUS_PLUS;
                    Advance();
                }
                else
                {
                    lexeme = "+";
                    type = TokenType.PLUS;
                }
                break;
            case '-':
                if (Peek() == '-')
                {
                    lexeme = "--";
                    type = TokenType.MINUS_MINUS;
                    Advance();
                }
                else
                {
                    lexeme = "-";
                    type = TokenType.MINUS;
                }
                break;
            case '=':
                if (Peek() == '=')
                {
                    lexeme = "==";
                    type = TokenType.EQUAL_EQUAL;
                    Advance();
                }
                else
                {
                    lexeme = "=";
                    type = TokenType.EQUAL;
                }
                break;
            case '!':
                if (Peek() == '=')
                {
                    lexeme = "!=";
                    type = TokenType.NOT_EQUAL;
                    Advance();
                }
                else
                {
                    lexeme = "!";
                    type = TokenType.NOT;
                }
                break;
            case '>':
                if (Peek() == '=')
                {
                    lexeme = ">=";
                    type = TokenType.GREATER_EQUAL;
                    Advance();
                }
                else
                {
                    lexeme = ">";
                    type = TokenType.GREATER;
                }
                break;
            case '<':
                if (Peek() == '=')
                {
                    lexeme = "<=";
                    type = TokenType.LESS_EQUAL;
                    Advance();
                }
                else
                {
                    lexeme = "<";
                    type = TokenType.LESS;
                }
                break;
            case '&':
                if (Peek() == '&')
                {
                    lexeme = "&&";
                    type = TokenType.AND_AND;
                    Advance();
                }
                else
                {
                    lexeme = "&";
                    type = TokenType.AND;
                }
                break;
            case '|':
                if (Peek() == '|')
                {
                    lexeme = "||";
                    type = TokenType.OR_OR;
                    Advance();
                }
                else
                {
                    lexeme = "|";
                    type = TokenType.OR;
                }
                break;
            case '*':
                lexeme = "*";
                type = TokenType.STAR;
                break;
            case '/':
                lexeme = "/";
                type = TokenType.SLASH;
                break;
            case '%':
                lexeme = "%";
                type = TokenType.PERCENT;
                break;
            case '^':
                lexeme = "^";
                type = TokenType.XOR;
                break;
            case '~':
                lexeme = "~";
                type = TokenType.TILDE;
                break;
            case '(':
                lexeme = "(";
                type = TokenType.LEFT_PAREN;
                break;
            case ')':
                lexeme = ")";
                type = TokenType.RIGHT_PAREN;
                break;
            case '{':
                lexeme = "{";
                type = TokenType.LEFT_BRACE;
                break;
            case '}':
                lexeme = "}";
                type = TokenType.RIGHT_BRACE;
                break;
            case '[':
                lexeme = "[";
                type = TokenType.LEFT_BRACKET;
                break;
            case ']':
                lexeme = "]";
                type = TokenType.RIGHT_BRACKET;
                break;
            case ';':
                lexeme = ";";
                type = TokenType.SEMICOLON;
                break;
            case ',':
                lexeme = ",";
                type = TokenType.COMMA;
                break;
            case '.':
                lexeme = ".";
                type = TokenType.DOT;
                break;
            case ':':
                lexeme = ":";
                type = TokenType.COLON;
                break;
            case '?':
                lexeme = "?";
                type = TokenType.QUESTION;
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
