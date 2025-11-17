# Cythonic Lexer

Production-ready lexical analyser for the case-insensitive Cythonic language (`.cytho`), implemented in **C (C11 standard)**. The lexer emits a sequence of `Token` structs containing `type`, `lexeme` (normalized lowercase for identifiers/keywords), `raw`, `line`, and `column`. Keyword recognition, identifiers, numbers, operators, and delimiters are all driven by explicit character-level DFAs—no full-string comparisons or string-key maps are used.

## What this project gives you
- A **tokenizer (lexer)** that reads `.cytho` source text and breaks it into meaningful tokens.
- A **command-line tool** that lets you run `./cythonic-lexer <file>` to see the token list for any `.cytho` file.
- **Robust error handling** - creates `INVALID` tokens for unrecognized characters instead of crashing.
- **Unterminated string support** - strings without closing quotes are valid (reads until newline/EOF).
- A **sample program** (`samples/sample.cytho`) demonstrating all language features (293 lines, 27 sections).
- A **state-table description** of the keyword DFA (168 states) for verifying and extending keyword recognition.
- **Symbol table generation** - automatically creates `.symboltable.txt` files with formatted token tables.

If you are new to lexers, think of this repository as the "front door" of the language pipeline: it reads raw text and turns it into structured pieces that later stages (parser, semantic analyser, compiler) can understand.

## Build & Run

**Requirements:**
- GCC compiler (MinGW on Windows, or standard GCC on Linux/Mac)
- Windows users: Install [MinGW](http://mingw.org/) or use [MSYS2](https://www.msys2.org/)

**Building:**
```bash
cd src/CythonicLexer
mingw32-make        # On Windows with MinGW
# OR
make                # On Linux/Mac
```

**Running:**
```bash
./cythonic-lexer ../../samples/sample.cytho       # Tokenize a file
mingw32-make run                                   # Quick run on sample file
mingw32-make test                                  # Generate symbol table
mingw32-make clean                                 # Remove build artifacts
```

**Outputs:**
- Token stream printed to console (one token per line with type, lexeme, line, column)
- Symbol table file: `<filename>.symboltable.txt` (formatted table of all tokens)

## How to try it out
1. Open a terminal in `src/CythonicLexer` directory.
2. Run `mingw32-make run` (Windows) or `make run` (Linux/Mac).
3. The program prints each token showing: `LINE:COL TYPE lexeme raw`
4. A symbol table file is automatically generated in the samples directory.
5. Edit or create your own `.cytho` file and run: `./cythonic-lexer yourfile.cytho`

## Key Features

### C-Specific Enhancements
- **INVALID Token Type**: Unrecognized characters (like `@`, `#`, `$`) create `INVALID` tokens instead of causing crashes
- **Unterminated Strings**: Strings without closing quotes are valid - the lexer reads until newline or EOF
- **Non-Fatal Error Handling**: Lexer continues processing after errors, collecting all tokens and issues
- **Memory Management**: Dynamic memory allocation for source text, proper cleanup on exit
- **Symbol Table Generation**: Automatically creates formatted token tables for debugging

### Sample Output
```
Input: if at (counter > 0) then { print("Hello"); }

Output:
1:1 RESERVED_WORD if if
1:4 NOISE_WORD at at
1:7 DELIMITER ( (
1:8 IDENTIFIER counter counter
1:16 OPERATOR > >
1:18 NUMBER 0 0
1:19 DELIMITER ) )
1:21 NOISE_WORD then then
1:26 DELIMITER { {
1:28 IDENTIFIER print print
1:33 DELIMITER ( (
1:34 STRING_LITERAL Hello "Hello"
1:41 DELIMITER ) )
1:42 DELIMITER ; ;
1:44 DELIMITER } }
1:45 EOF  
```

## Sample Tokenization

The `samples/sample.cytho` file contains a **comprehensive demonstration** of all Cythonic language features:
- All 12 token types (KEYWORD, RESERVED_WORD, TYPE, IDENTIFIER, NUMBER, STRING_LITERAL, CHAR_LITERAL, OPERATOR, DELIMITER, BOOLEAN_LITERAL, COMMENT, NOISE_WORD, EOF, INVALID)
- 21 Contextual Keywords (KEYWORD): and, args, async, dyn, get, global, init, let, nmof, nnull, or, rec, req, set, stc, str, struct, switch, this, val, var
- 33 Reserved Words (RESERVED_WORD): as, base, bool, break, case, char, class, default, do, double, else, enum, false, for, foreach, if, iface, in, int, new, next, nspace, null, num, priv, prot, pub, rdo, record, return, true, use, void, while
- 3 Noise Words (NOISE_WORD): at, its, then
- 7 Types (TYPE): bool, char, int, double, str, num, void
- All operators (arithmetic, logical, bitwise, comparison, assignment)
- All delimiters and brackets
- String/char literals with escape sequences
- Single-line and multi-line comments
- Nested control structures with noise words

Input (excerpt from `samples/sample.cytho`):

```cytho
// Noise words demonstration
pub void noiseWordDemo() {
    // Noise word "at" in if statement
    if at (counter > 0) then {
        print("Using 'at' and 'then' noise words");
    }
    
    // Noise word "its" in while loop
    while its (counter < 100) {
        counter++;
    }
    
    // Without noise words (they are optional)
    if (counter == 50) {
        print("No noise words here");
    }
}
```

## Keyword DFA State Table (168 states, 57 keywords)

**Note**: This trie includes 21 contextual keywords (KEYWORD), 33 reserved words (RESERVED_WORD), and 3 noise words (NOISE_WORD).

```
STATE | ACCEPT | TYPE            | TRANSITIONS
--------------------------------------------------------------------------------
    0 |  no   | -               | a->1,b->11,c->22,d->33,e->47,f->54,g->65,i->73,l->82,n->85,o->106,p->108,r->116,s->129,t->143,u->150,v->153,w->160
    1 |  no   | -               | n->2,r->4,s->7,t->165
    2 |  no   | -               | d->3
    3 | yes   | KEYWORD         | (and)
    4 |  no   | -               | g->5
    5 |  no   | -               | s->6
    6 | yes   | KEYWORD         | (args)
    7 | yes   | RESERVED_WORD   | (as) y->8
    8 |  no   | -               | n->9
    9 |  no   | -               | c->10
   10 | yes   | KEYWORD         | (async)
   ...
  165 | yes   | NOISE_WORD      | (at)
  167 | yes   | NOISE_WORD      | (its)
  168 | yes   | NOISE_WORD      | (then)
```

**Full DFA states**: 168 total states recognizing all keywords, reserved words, and noise words with case-insensitive matching.

## Operator DFA (Longest-Match)

```
State OP0 (start)
	'+' -> OP_PLUS1, '-' -> OP_MINUS1, '=' -> OP_EQ1, '!' -> OP_BANG1,
	'>' -> OP_GT1, '<' -> OP_LT1, '&' -> OP_AMP1, '|' -> OP_BAR1,
	other operator characters -> emit first character as a standalone operator/delimiter
State OP_PLUS1: '+' -> emit "++"; otherwise emit '+'
State OP_MINUS1: '-' -> emit "--"; otherwise emit '-'
State OP_EQ1: '=' -> emit "=="; otherwise emit '='
State OP_BANG1: '=' -> emit "!="; otherwise emit '!'
State OP_GT1: '=' -> emit ">="; otherwise emit '>'
State OP_LT1: '=' -> emit "<="; otherwise emit '<'
State OP_AMP1: '&' -> emit "&&"; otherwise emit '&'
State OP_BAR1: '|' -> emit "||"; otherwise emit '|'
```

## Tokenisation Notes
- **Keywords vs Reserved Words**: Keywords (21) are contextual and can be used as identifiers in some contexts. Reserved words (33) are strictly reserved. Both are recognized by the same DFA trie.
- **Noise words** (`at`, `its`, `then`) are optional filler words that enhance readability but have no semantic meaning. They are tokenized as `NOISE_WORD` type.
- **Case-insensitive**: All keywords, identifiers, and noise words are normalized to lowercase in the `lexeme` field, while preserving original case in `raw`.
- **INVALID tokens**: Unrecognized characters like `@`, `#`, `$` produce `INVALID` tokens instead of crashing.
- **Unterminated strings**: Strings without closing `"` are valid - the lexer reads until newline or EOF. This differs from strict implementations.
- **Position tracking**: Line and column numbers are accurately tracked, with escape sequences counted as single characters for column purposes.
- **Numeric DFA**: Supports integers (`123`), floats (`0.5`, `.5`, `10.`), and scientific notation (`1e10`, `1.23e-4`).
- **String escapes**: Recognizes `\n`, `\t`, `\\`, `\"`, `\'`, `\r`, `\b`, `\f`, `\0`.
- **Comments**: Single-line (`//`) and multi-line (`/* */`) comments are tokenized as `COMMENT` tokens with content normalized to lowercase.
- **Operators**: Longest-match semantics for multi-character operators (`>=`, `==`, `++`, `&&`, `||`).
- **Delimiters**: Punctuation `;`, `,`, `.`, `(`, `)`, `{`, `}`, `[`, `]`, `:`, `?` emit `DELIMITER` tokens.

## Project Layout
- `src/CythonicLexer/` – C lexer implementation
  - `Lexer.c` – Main lexer implementation (1,200+ lines)
  - `Makefile` – Build system for GCC/MinGW
  - `LEXER_C_README.md` – Detailed C implementation documentation
- `samples/` – Test files
  - `sample.cytho` – Comprehensive sample demonstrating all features (293 lines)
  - `noise_words_test.cytho` – Dedicated noise word tests
- `COMPLIANCE.md` – Language compliance report

## Implementation Details

### Token Structure
```c
typedef struct {
    TokenType type;
    char lexeme[32];   // Normalized (lowercase), max 31 chars + null
    char raw[64];      // Original text as written, max 63 chars + null
    int line;
    int column;
} Token;
```

### Keyword Trie Structure
```c
typedef struct KeywordNode {
    int transitions[26];      // a-z transitions
    bool is_accepting;        // Is this an accepting state?
    TokenType accepting_type; // KEYWORD, RESERVED_WORD, TYPE, etc.
} KeywordNode;
```

### Lexer Structure
```c
typedef struct {
    char *source;        // Source code string
    size_t position;     // Current position in source
    int line;            // Current line number (1-indexed)
    int column;          // Current column number (1-indexed)
    char current;        // Current character
    char peek;           // Next character (lookahead)
} Lexer;
```

## Lexer Flow

```
Source Text Input → Character Stream → Token Recognition → Token List Output
     ↓                    ↓                    ↓                    ↓
  "if (x)"         Scan char by char    Identify patterns    [KEYWORD,DELIMITER,IDENTIFIER,DELIMITER,EOF]
```

### Processing Steps:
1. **Initialization**: Load source text, build keyword trie (168 states)
2. **Main Loop**: Skip whitespace, identify token start, classify first character
3. **Token Recognition**: 
   - `//` or `/*` → Comment
   - Letter/underscore → Identifier or Keyword (DFA)
   - Digit → Number
   - Quote → String/Char literal
   - Operator char → Operator/Delimiter (longest match)
   - Unknown → INVALID token
4. **Position Tracking**: Update line/column after each character
5. **Output**: Emit token with type, lexeme, raw, line, column

## Key Design Principles

| Feature | Benefit |
|---------|---------|
| **DFA keyword recognition** | O(n) time, no hash lookups |
| **Position tracking** | Accurate error reporting |
| **INVALID tokens** | Graceful error recovery |
| **Unterminated strings** | Flexible parsing for incomplete input |
| **Case-insensitive** | User-friendly (COUNT = count) |
| **Longest match** | Correct multi-char operators (>= not > =) |
| **No string maps** | Constant memory per character |

This architecture ensures **O(n) time complexity** with **constant memory per character**, making it production-ready for large source files.

## Documentation
- See `src/CythonicLexer/LEXER_C_README.md` for detailed C implementation guide
- See `COMPLIANCE.md` for language specification compliance
- See `samples/sample.cytho` for comprehensive language examples
