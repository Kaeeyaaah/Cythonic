using System.Collections.Generic;

namespace CythonicLexer;

/// <summary>
/// Implements a DFA-based trie used to classify keywords without string comparisons.
/// </summary>
internal sealed class KeywordTrie
{
    private sealed class KeywordNode
    {
        public KeywordNode()
        {
            Transitions = new int[26];
            for (var i = 0; i < Transitions.Length; i++)
            {
                Transitions[i] = -1;
            }
        }

        public int[] Transitions { get; }
        public bool IsAccepting { get; set; }
        public TokenType AcceptingType { get; set; }
    }

    private readonly List<KeywordNode> _nodes = new();

    public KeywordTrie()
    {
        _nodes.Add(new KeywordNode());
        foreach (var entry in KeywordEntries)
        {
            Add(entry.Text, entry.Type);
        }
    }

    private static readonly KeywordEntry[] KeywordEntries =
    {
        // Contextual Keywords (21)
        new("and", TokenType.KEYWORD),
        new("args", TokenType.KEYWORD),
        new("async", TokenType.KEYWORD),
        new("dyn", TokenType.KEYWORD),
        new("get", TokenType.KEYWORD),
        new("global", TokenType.KEYWORD),
        new("init", TokenType.KEYWORD),
        new("let", TokenType.KEYWORD),
        new("nmof", TokenType.KEYWORD),
        new("nnull", TokenType.KEYWORD),
        new("or", TokenType.KEYWORD),
        new("rec", TokenType.KEYWORD),
        new("req", TokenType.KEYWORD),
        new("set", TokenType.KEYWORD),
        new("stc", TokenType.KEYWORD),
        new("str", TokenType.TYPE),
        new("struct", TokenType.KEYWORD),
        new("switch", TokenType.KEYWORD),
        new("this", TokenType.KEYWORD),
        new("val", TokenType.KEYWORD),
        new("var", TokenType.KEYWORD),
        
        // Reserved Words (33)
        new("as", TokenType.RESERVED_WORD),
        new("base", TokenType.RESERVED_WORD),
        new("bool", TokenType.TYPE),
        new("break", TokenType.RESERVED_WORD),
        new("case", TokenType.RESERVED_WORD),
        new("char", TokenType.TYPE),
        new("class", TokenType.RESERVED_WORD),
        new("default", TokenType.RESERVED_WORD),
        new("do", TokenType.RESERVED_WORD),
        new("double", TokenType.TYPE),
        new("else", TokenType.RESERVED_WORD),
        new("enum", TokenType.RESERVED_WORD),
        new("false", TokenType.BOOLEAN_LITERAL),
        new("for", TokenType.RESERVED_WORD),
        new("foreach", TokenType.RESERVED_WORD),
        new("if", TokenType.RESERVED_WORD),
        new("iface", TokenType.RESERVED_WORD),
        new("in", TokenType.RESERVED_WORD),
        new("int", TokenType.TYPE),
        new("new", TokenType.RESERVED_WORD),
        new("next", TokenType.RESERVED_WORD),
        new("nspace", TokenType.RESERVED_WORD),
        new("null", TokenType.RESERVED_WORD),
        new("num", TokenType.TYPE),
        new("priv", TokenType.RESERVED_WORD),
        new("prot", TokenType.RESERVED_WORD),
        new("pub", TokenType.RESERVED_WORD),
        new("rdo", TokenType.RESERVED_WORD),
        new("record", TokenType.RESERVED_WORD),
        new("return", TokenType.RESERVED_WORD),
        new("true", TokenType.BOOLEAN_LITERAL),
        new("use", TokenType.RESERVED_WORD),
        new("void", TokenType.TYPE),
        new("while", TokenType.RESERVED_WORD),
        
        // Noise Words (3)
        new("at", TokenType.NOISE_WORD),
        new("its", TokenType.NOISE_WORD),
        new("then", TokenType.NOISE_WORD)
    };

    private void Add(string text, TokenType type)
    {
        var state = 0;
        foreach (var raw in text)
        {
            var lower = AsciiLower(raw);
            var index = lower - 'a';
            if ((uint)index >= 26)
            {
                throw new InvalidOperationException($"Keyword '{text}' contains unsupported character '{raw}'.");
            }

            var next = _nodes[state].Transitions[index];
            if (next == -1)
            {
                next = _nodes.Count;
                _nodes[state].Transitions[index] = next;
                _nodes.Add(new KeywordNode());
            }

            state = next;
        }

        _nodes[state].IsAccepting = true;
        _nodes[state].AcceptingType = type;
    }

    public int Move(int state, char lowerCaseLetter)
    {
        if ((uint)state >= _nodes.Count)
        {
            return -1;
        }

        var index = lowerCaseLetter - 'a';
        if ((uint)index >= 26)
        {
            return -1;
        }

        return _nodes[state].Transitions[index];
    }

    public bool TryGetAcceptingType(int state, out TokenType tokenType)
    {
        if ((uint)state < _nodes.Count && _nodes[state].IsAccepting)
        {
            tokenType = _nodes[state].AcceptingType;
            return true;
        }

        tokenType = default;
        return false;
    }

    private static char AsciiLower(char c)
    {
        if (c >= 'A' && c <= 'Z')
        {
            return (char)(c | 0x20);
        }

        return c;
    }

    private readonly record struct KeywordEntry(string Text, TokenType Type);
}
