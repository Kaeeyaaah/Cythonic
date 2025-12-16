# Cythonic Compiler

Production-ready compiler for the case-insensitive Cythonic language (`.cytho`), implemented in **C (C11 standard)**. The compiler includes a **lexical analyzer** (tokenizer) and a **recursive descent parser** (syntax analyzer) that processes `.cytho` source files and generates symbol tables and parse trees.

## What this project gives you
- A **tokenizer (lexer)** that reads `.cytho` source text and breaks it into meaningful tokens using DFA-based keyword recognition.
- A **recursive descent parser** that performs syntax analysis and generates detailed parse trees.
- A **command-line tool** that lets you run `./cythonic <file>` to analyze any `.cytho` file.
- **Robust error handling** - creates `INVALID` tokens for unrecognized characters and provides detailed syntax error messages.
- **Unterminated string support** - strings without closing quotes are valid (reads until newline/EOF).
- **Sample programs** demonstrating language features and compiler capabilities.
- **Symbol table generation** - automatically creates `.symboltable.txt` files with formatted token tables.
- **Parse tree generation** - automatically creates `.parsetree.txt` files showing derivation trees.

If you are new to lexers, think of this repository as the "front door" of the language pipeline: it reads raw text and turns it into structured pieces that later stages (parser, semantic analyser, compiler) can understand.

## Build & Run

**Requirements:**
- GCC compiler (MinGW on Windows, or standard GCC on Linux/Mac)
- Windows users: Install [MinGW](http://mingw.org/) or use [MSYS2](https://www.msys2.org/)

**Building:**
```bash
cd src
gcc Cythonic.c -o cythonic.exe -Wall -Wextra -std=c11    # Windows
# OR
gcc Cythonic.c -o cythonic -Wall -Wextra -std=c11        # Linux/Mac
```

**Running:**
```bash
./cythonic ../samples/sample.cytho           # Analyze a file
./cythonic ../samples/valid_syntax.cytho     # Test with valid syntax
./cythonic ../samples/test_new_keywords.cytho # Test keywords and operators
```

**Outputs:**
- Symbol table file: `<filename>.symboltable.txt` (formatted table of all tokens)
- Parse tree file: `<filename>.parsetree.txt` (detailed derivation tree)
- Console output: Syntax analysis results with error reporting

## How to try it out
1. Open a terminal in `src` directory.
2. Compile: `gcc Cythonic.c -o cythonic.exe -Wall -Wextra -std=c11`
3. Run: `./cythonic.exe ../samples/valid_syntax.cytho`
4. Two files are automatically generated:
   - `valid_syntax.cytho.symboltable.txt` - Token table
   - `valid_syntax.cytho.parsetree.txt` - Parse tree
5. Edit or create your own `.cytho` file and run: `./cythonic yourfile.cytho`

## Key Features

### Compiler Features
- **Two-Phase Analysis**: Lexical analysis (tokenization) followed by syntax analysis (parsing)
- **INVALID Token Type**: Unrecognized characters (like `@`, `#`, `$`) create `INVALID` tokens instead of causing crashes
- **Unterminated Strings**: Strings without closing quotes are valid - the lexer reads until newline or EOF
- **Non-Fatal Error Handling**: Compiler continues processing after errors, collecting all tokens and issues
- **Memory Management**: Dynamic memory allocation for source text, proper cleanup on exit
- **Symbol Table Generation**: Automatically creates formatted token tables for debugging
- **Parse Tree Generation**: Produces detailed derivation trees showing grammar rule applications
- **Panic-Mode Error Recovery**: Parser can recover from errors and continue analysis

### Sample Output
```
Input File (test.cytho):
if at (x > 0) then {
    print("Positive");
}

Console Output:
Symbol table written to: test.cytho.symboltable.txt
Writing parse tree to: test.cytho.parsetree.txt
Starting Syntax Analysis...
Syntax Analysis Complete: No errors found.

Symbol Table (test.cytho.symboltable.txt):
LINE | COL | TYPE              | LEXEME                        | RAW
-----|-----|-------------------|-------------------------------|------------------
   1 |   1 | RESERVED_WORD     | if                            | if
   1 |   4 | NOISE_WORD        | at                            | at
   1 |   7 | LEFT_PAREN        | (                             | (
   1 |   8 | IDENTIFIER        | x                             | x
   1 |  10 | GREATER           | >                             | >
   1 |  12 | NUMBER            | 0                             | 0
   1 |  13 | RIGHT_PAREN       | )                             | )
   1 |  15 | NOISE_WORD        | then                          | then
   1 |  20 | LEFT_BRACE        | {                             | {
   2 |   5 | KEYWORD           | print                         | print
   2 |  10 | LEFT_PAREN        | (                             | (
   2 |  11 | STRING_LITERAL    | Positive                      | "Positive"
   2 |  21 | RIGHT_PAREN       | )                             | )
   2 |  22 | SEMICOLON         | ;                             | ;
   3 |   1 | RIGHT_BRACE       | }                             | }

Parse Tree (excerpt from test.cytho.parsetree.txt):
Enter <Statement>
  Enter <IfStatement>
    Enter <Expression>
      Enter <LogicalOr>
        Enter <LogicalAnd>
          Enter <Equality>
            Enter <Comparison>
              ...
```

## Language Features

The Cythonic compiler recognizes and processes:
- **Token Types**: KEYWORD, RESERVED_WORD, TYPE, IDENTIFIER, NUMBER, STRING_LITERAL, CHAR_LITERAL, OPERATOR, DELIMITER, BOOLEAN_LITERAL, COMMENT, NOISE_WORD, EOF, INVALID
- **24 Contextual Keywords (KEYWORD)**: and, args, async, const, dyn, get, global, init, input, let, nmof, nnull, or, print, rec, req, set, stc, struct, switch, this, val, var, where
- **26 Reserved Words (RESERVED_WORD)**: as, base, break, case, class, default, do, else, enum, for, foreach, if, iface, in, new, next, nspace, null, priv, prot, pub, rdo, record, return, use, while
- **7 Type Keywords (TYPE)**: bool, char, int, double, str, num, void
- **2 Boolean Literals**: true, false
- **3 Noise Words (NOISE_WORD)**: at, its, then (optional readability enhancers)
- **Operators**: 
  - Arithmetic: `+`, `-`, `*`, `/`, `%`, `++`, `--`
  - Assignment: `=`, `+=`, `-=`, `*=`, `/=`, `%=`
  - Comparison: `==`, `!=`, `>`, `<`, `>=`, `<=`
  - Logical: `&&`, `||`, `!`
  - Bitwise: `&`, `|`, `^`, `~`
- **Delimiters**: `(`, `)`, `{`, `}`, `[`, `]`, `;`, `,`, `.`, `:`, `?`
- **Grammar Support**:
  - Variable declarations with optional type inference (`var`, `const`, `dyn`)
  - Assignments with compound operators
  - Control flow: `if`/`else`, `while`, `for`
  - I/O statements: `input()`, `print()`
  - Expressions with full operator precedence
  - Blocks and nested statements

### Sample Code (from `samples/test_new_keywords.cytho`):

```cytho
// Test new keywords: where, const, input, print
const int x = 10;
var y = 5;

input(y);
print("Value: ");
print(y);

// Test logical operators
if (x > 5 && y < 20) {
    print("Both conditions true");
}

if (x == 10 || y == 0) {
    print("At least one condition true");
}
```

### Noise Words Example:

```cytho
// Noise words are optional readability enhancers
if at (counter > 0) then {
    print("Using 'at' and 'then' noise words");
}

while its (counter < 100) {
    counter++;
}

// Same code without noise words (both valid)
if (counter > 0) {
    print("No noise words here");
}
```

## Parser Grammar

The recursive descent parser implements the following grammar with proper operator precedence:

### Expression Precedence (highest to lowest):
1. **Primary**: literals, identifiers, parenthesized expressions
2. **Postfix**: `++`, `--`
3. **Unary**: `!`, `-` (negation)
4. **Factor**: `*`, `/`, `%`
5. **Term**: `+`, `-`
6. **Comparison**: `>`, `<`, `>=`, `<=`
7. **Equality**: `==`, `!=`
8. **Logical AND**: `&&`
9. **Logical OR**: `||`

### Statement Grammar:
```
Program          → Statement*
Statement        → Declaration | Assignment | Input | Output | 
                   If | While | For | Return | Block
Declaration      → (TYPE | var | const | dyn) [TYPE] IDENTIFIER [= Expression] ;
Assignment       → IDENTIFIER (= | += | -= | *= | /= | %=) Expression ;
Input            → input ( IDENTIFIER ) ;
Output           → print ( Expression ) ;
If               → if [at] ( Expression ) [then] Statement [else Statement]
While            → while [its] ( Expression ) Statement
For              → for ( (Declaration | Assignment | ;) [Expression] ; [Expression] ) Statement
Return           → return [Expression] ;
Block            → { Statement* }
```

**Note**: `[at]`, `[its]`, `[then]` are optional noise words for readability.## Operator Recognition (Longest-Match)

The lexer uses longest-match semantics to correctly tokenize multi-character operators:

| Input | Tokens | Notes |
|-------|--------|-------|
| `x+=5` | `IDENTIFIER`, `PLUS_EQUAL`, `NUMBER` | Compound assignment |
| `x++` | `IDENTIFIER`, `PLUS_PLUS` | Postfix increment |
| `x+y` | `IDENTIFIER`, `PLUS`, `IDENTIFIER` | Binary addition |
| `x==y` | `IDENTIFIER`, `EQUAL_EQUAL`, `IDENTIFIER` | Equality comparison |
| `x=y` | `IDENTIFIER`, `EQUAL`, `IDENTIFIER` | Assignment |
| `x&&y` | `IDENTIFIER`, `AND_AND`, `IDENTIFIER` | Logical AND |
| `x&y` | `IDENTIFIER`, `AND`, `IDENTIFIER` | Bitwise AND |

**Critical**: The lexer checks compound operators (`+=`, `-=`, `*=`, `/=`, `%=`) **before** single-character operators to ensure correct tokenization.

## Implementation Notes

### Lexical Analysis
- **Keywords vs Reserved Words**: Keywords (24) are contextual, reserved words (26) are strictly reserved. Both use the same Trie-based DFA for O(n) recognition.
- **Noise words** (`at`, `its`, `then`) are optional readability enhancers with no semantic meaning, tokenized as `NOISE_WORD`.
- **Case-insensitive**: All keywords, identifiers, and noise words normalized to lowercase in `lexeme`, original case preserved in `raw`.
- **INVALID tokens**: Unrecognized characters like `@`, `#`, `$` produce `INVALID` tokens instead of crashing.
- **Unterminated strings**: Strings without closing `"` are valid - lexer reads until newline or EOF.
- **Position tracking**: Line and column numbers accurately tracked for error reporting.
- **Numeric support**: Integers (`123`), floats (`0.5`, `.5`, `10.`), scientific notation (`1e10`, `1.23e-4`).
- **String escapes**: `\n`, `\t`, `\\`, `\"`, `\'`, `\r`, `\b`, `\f`, `\0`.
- **Comments**: Single-line (`//`) and multi-line (`/* */`) tokenized as `COMMENT`.
- **Operators**: Longest-match semantics for multi-character operators (`>=`, `==`, `++`, `&&`, `||`, `+=`, `-=`, etc.).

### Syntax Analysis
- **Recursive descent**: Top-down parsing with one function per non-terminal.
- **Operator precedence**: Proper precedence chain from primary → postfix → unary → factor → term → comparison → equality → logical_and → logical_or.
- **Error recovery**: Panic-mode recovery continues parsing after errors to report multiple issues.
- **Parse tree**: Detailed derivation tree shows all grammar rule applications.
- **Noise word handling**: Parser optionally consumes noise words in `if` and `while` statements.

## Project Layout
```
Cythonic/
├── src/
│   ├── Cythonic.c              # Main compiler (lexer + parser)
│   ├── Makefile                # Build configuration
│   ├── ParsingTable.md         # LL(1) parsing table documentation
│   └── TransitionDiagram.mermaid # State diagram visualization
├── samples/
│   ├── sample.cytho            # Comprehensive language demo (293 lines)
│   ├── valid_syntax.cytho      # Valid syntax test suite
│   ├── test_new_keywords.cytho # Keyword and operator tests
│   └── test_compound_ops.cytho # Compound assignment tests
└── README.md                   # This file
```

## Core Data Structures

### Token
```c
typedef struct {
    TokenType type;    // Token classification
    char* lexeme;      // Normalized text (lowercase)
    char* raw;         // Original text as written
    int line;          // Line number (1-indexed)
    int column;        // Column number (1-indexed)
} Token;
```

### Keyword Trie (DFA)
```c
typedef struct {
    int transitions[26];       // a-z transitions
    bool is_accepting;         // Accepting state?
    TokenType accepting_type;  // KEYWORD, RESERVED_WORD, TYPE, etc.
} TrieNode;

typedef struct {
    TrieNode nodes[200];  // 200 states for all keywords
    int node_count;
} KeywordTrie;
```

### Lexer
```c
typedef struct {
    const char* source;  // Source code
    int length;          // Source length
    int index;           // Current position
    int line;            // Current line (1-indexed)
    int column;          // Current column (1-indexed)
    KeywordTrie* trie;   // Keyword DFA
} Lexer;
```

### Parser
```c
typedef struct {
    Token* tokens;           // Token array
    int token_count;         // Number of tokens
    int current;             // Current token index
    Token current_token;     // Current token
    Token previous_token;    // Previous token
    bool panic_mode;         // Error recovery state
    int error_count;         // Number of errors
    FILE* parse_tree_file;   // Parse tree output
    int depth;               // Parse tree depth
} Parser;
```

## Compiler Pipeline

```
Source File (.cytho)
        ↓
┌───────────────────┐
│ LEXICAL ANALYSIS  │  Character stream → Token stream
│  (Tokenization)   │  - Keyword DFA recognition
│                   │  - Operator longest-match
└────────┬──────────┘  - Position tracking
         ↓
   Token Stream
         ↓
┌───────────────────┐
│ SYNTAX ANALYSIS   │  Token stream → Parse tree
│ (Recursive Descent│  - Grammar validation
│     Parser)       │  - Error recovery
└────────┬──────────┘  - Derivation tree
         ↓
┌─────────────────────────────────┐
│         OUTPUT FILES            │
│  1. Symbol Table (.symboltable) │
│  2. Parse Tree (.parsetree)     │
│  3. Console Error Report        │
└─────────────────────────────────┘
```

### Processing Steps:
1. **Load Source**: Read `.cytho` file into memory
2. **Build Keyword Trie**: Initialize DFA with 200 states for all keywords
3. **Tokenization**: 
   - Skip whitespace, track line/column
   - Recognize patterns: comments, identifiers, keywords, numbers, strings, operators
   - Generate token list with position information
4. **Parsing**: 
   - Build recursive descent parser from token stream
   - Apply grammar rules with precedence climbing
   - Generate parse tree with error recovery
5. **Output**: 
   - Write symbol table (formatted token list)
   - Write parse tree (derivation structure)
   - Report syntax errors with line/column

## Design Principles

| Principle | Implementation | Benefit |
|-----------|----------------|---------|
| **DFA-based lexing** | Trie keyword recognition | O(n) tokenization, no hash lookups |
| **Longest match** | Check compound ops before single | Correct tokenization of `+=` vs `+` |
| **Recursive descent** | One function per non-terminal | Clear grammar mapping, maintainable |
| **Operator precedence** | Precedence climbing | Correct expression evaluation order |
| **Error recovery** | Panic mode with synchronization | Multiple errors reported per run |
| **Position tracking** | Line/column in every token | Precise error messages |
| **Case insensitivity** | Normalize to lowercase | User-friendly (`IF` = `if`) |
| **Single-pass compilation** | Lexer + parser in one pass | Efficient processing |

**Complexity**: O(n) time, O(n) space where n = source file size

## Future Enhancements

The current parser supports basic control flow and expressions. Planned features:
- ✅ Compound assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`)
- ✅ Logical operators (`&&`, `||`)
- ✅ Keywords: `const`, `input`, `print`, `where`
- ⚠️ Advanced features (recognized as tokens, parser not implemented):
  - Class definitions (`class`, `iface`, `record`, `struct`, `enum`)
  - Namespace declarations (`nspace`, `use`)
  - Advanced loops (`foreach`, `switch`)
  - Function definitions and calls
  - Property accessors (`get`, `set`, `init`)
  - Advanced types and generics

## Documentation
- `src/ParsingTable.md` - LL(1) parsing table and grammar
- `src/TransitionDiagram.mermaid` - Lexer state diagram
- `samples/*.cytho` - Test cases and examples
