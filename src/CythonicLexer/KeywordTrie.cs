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
        new("and", TokenType.KEYWORD),
        new("args", TokenType.KEYWORD),
        new("as", TokenType.KEYWORD),
        new("async", TokenType.KEYWORD),
        new("at", TokenType.NOISE_WORD),
        new("base", TokenType.KEYWORD),
        new("bool", TokenType.TYPE),
        new("break", TokenType.KEYWORD),
        new("case", TokenType.KEYWORD),
        new("char", TokenType.TYPE),
        new("class", TokenType.KEYWORD),
        new("default", TokenType.KEYWORD),
        new("do", TokenType.KEYWORD),
        new("double", TokenType.TYPE),
        new("dyn", TokenType.KEYWORD),
        new("else", TokenType.KEYWORD),
        new("enum", TokenType.KEYWORD),
        new("false", TokenType.BOOLEAN_LITERAL),
        new("for", TokenType.KEYWORD),
        new("foreach", TokenType.KEYWORD),
        new("get", TokenType.KEYWORD),
        new("global", TokenType.KEYWORD),
        new("if", TokenType.KEYWORD),
        new("iface", TokenType.KEYWORD),
        new("in", TokenType.KEYWORD),
        new("init", TokenType.KEYWORD),
        new("int", TokenType.TYPE),
        new("its", TokenType.NOISE_WORD),
        new("let", TokenType.KEYWORD),
        new("next", TokenType.KEYWORD),
        new("new", TokenType.KEYWORD),
        new("nmof", TokenType.KEYWORD),
        new("nnull", TokenType.KEYWORD),
        new("nspace", TokenType.KEYWORD),
        new("null", TokenType.KEYWORD),
        new("num", TokenType.TYPE),
        new("or", TokenType.KEYWORD),
        new("priv", TokenType.KEYWORD),
        new("prot", TokenType.KEYWORD),
        new("pub", TokenType.KEYWORD),
        new("rdo", TokenType.KEYWORD),
        new("rec", TokenType.KEYWORD),
        new("record", TokenType.KEYWORD),
        new("req", TokenType.KEYWORD),
        new("return", TokenType.KEYWORD),
        new("set", TokenType.KEYWORD),
        new("stc", TokenType.KEYWORD),
        new("str", TokenType.TYPE),
        new("struct", TokenType.KEYWORD),
        new("switch", TokenType.KEYWORD),
        new("then", TokenType.NOISE_WORD),
        new("this", TokenType.KEYWORD),
        new("true", TokenType.BOOLEAN_LITERAL),
        new("use", TokenType.KEYWORD),
        new("val", TokenType.KEYWORD),
        new("var", TokenType.KEYWORD),
        new("void", TokenType.TYPE),
        new("while", TokenType.KEYWORD)
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
