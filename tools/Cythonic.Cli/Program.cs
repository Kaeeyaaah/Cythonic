using System.Text.Json;
using CythonicLexer;

var argsList = args.ToList();
if (argsList.Count == 0)
{
    PrintUsage();
    return;
}

var command = argsList[0].ToLowerInvariant();
if (command != "run")
{
    Console.Error.WriteLine($"Unknown command '{argsList[0]}'.");
    PrintUsage();
    Environment.ExitCode = 1;
    return;
}

if (argsList.Count < 2)
{
    Console.Error.WriteLine("Missing input file.");
    PrintUsage();
    Environment.ExitCode = 1;
    return;
}

var path = argsList[1];

try
{
    var lexer = Lexer.FromFile(path);
    var tokens = lexer.Lex();

    // Generate symbol table file
    var fullPath = Path.GetFullPath(path);
    var symbolTablePath = Path.ChangeExtension(fullPath, ".symboltable.txt");
    lexer.WriteSymbolTable(symbolTablePath);
    Console.WriteLine($"Symbol table written to: {symbolTablePath}");
    Console.WriteLine();

    var output = tokens
        .Where(t => t.Type != TokenType.EOF)
        .Select(t => new SimplifiedToken(t.Type.ToString(), t.Lexeme, t.Line, t.Column, t.Raw))
        .ToArray();

    var json = JsonSerializer.Serialize(output, new JsonSerializerOptions
    {
        WriteIndented = true
    });

    Console.WriteLine(json);
}
catch (FileNotFoundException ex)
{
    Console.Error.WriteLine(ex.Message);
    Environment.ExitCode = 1;
    return;
}
catch (ArgumentException ex)
{
    Console.Error.WriteLine(ex.Message);
    Environment.ExitCode = 1;
    return;
}
catch (LexerException ex)
{
    Console.Error.WriteLine($"Lexical error at line {ex.Line}, column {ex.Column}: {ex.Message}");
    Environment.ExitCode = 1;
    return;
}

return;

static void PrintUsage()
{
    const string usage = "Usage: cythonic run <path-to-.cytho>";
    Console.WriteLine(usage);
}

internal readonly record struct SimplifiedToken(string Type, string Lexeme, int Line, int Column, string Raw);
