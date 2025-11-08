using System.Collections;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using CythonicLexer;

var keywordTrieType = Type.GetType("CythonicLexer.KeywordTrie, CythonicLexer", throwOnError: true)
                      ?? throw new InvalidOperationException("Cannot locate keyword trie type");
var trie = Activator.CreateInstance(keywordTrieType, nonPublic: true)
           ?? throw new InvalidOperationException("Cannot instantiate keyword trie");

var nodesField = keywordTrieType.GetField("_nodes", BindingFlags.NonPublic | BindingFlags.Instance)
                 ?? throw new InvalidOperationException("Cannot locate keyword trie nodes");
var nodes = (IEnumerable)nodesField.GetValue(trie)!;
var keywordNodeType = keywordTrieType.GetNestedType("KeywordNode", BindingFlags.NonPublic)
                      ?? throw new InvalidOperationException("Cannot locate keyword node type");
var transitionsProperty = keywordNodeType.GetProperty("Transitions")
                           ?? throw new InvalidOperationException("Cannot locate transitions property");
var acceptProperty = keywordNodeType.GetProperty("IsAccepting")
                      ?? throw new InvalidOperationException("Cannot locate acceptance property");
var typeProperty = keywordNodeType.GetProperty("AcceptingType")
                   ?? throw new InvalidOperationException("Cannot locate type property");

var rows = new List<Row>();
var index = 0;
foreach (var node in nodes)
{
    var transitions = (int[])transitionsProperty.GetValue(node)!;
    var accepts = (bool)acceptProperty.GetValue(node)!;
    var tokenType = typeProperty.GetValue(node)?.ToString() ?? "-";

    var edges = new List<Edge>();
    for (var i = 0; i < transitions.Length; i++)
    {
        var next = transitions[i];
        if (next != -1)
        {
            edges.Add(new Edge((char)('a' + i), next));
        }
    }

    rows.Add(new Row(index, accepts, tokenType, edges));
    index++;
}

var builder = new StringBuilder();
builder.AppendLine("STATE | ACCEPT | TYPE            | TRANSITIONS");
builder.AppendLine(new string('-', 80));
foreach (var row in rows)
{
    var typeLabel = row.Accept ? row.Type : "-";
    var edgeText = row.Edges.Count == 0
        ? string.Empty
        : string.Join(",", row.Edges.Select(e => $"{e.Letter}->{e.Target}"));
    builder.AppendLine($"{row.State,5} | {(row.Accept ? "yes" : " no")}   | {typeLabel,-15} | {edgeText}");
}

Console.WriteLine(builder.ToString());

var samplePath = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "..", "samples", "sample.cytho"));
var sampleSource = File.ReadAllText(samplePath);
var lexerTokens = new Lexer(sampleSource).Lex().Where(t => t.Type != TokenType.EOF).ToList();

Console.WriteLine("SAMPLE TOKENS");
foreach (var token in lexerTokens)
{
    Console.WriteLine($"{token.Line,4}:{token.Column,3} {token.Type,-16} {token.Lexeme,-12} {token.Raw}");
}

record struct Edge(char Letter, int Target);
record struct Row(int State, bool Accept, string Type, IReadOnlyList<Edge> Edges);
