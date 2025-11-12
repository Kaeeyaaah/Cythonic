# Cythonic Lexer

Production-ready lexical analyser for the case-insensitive Cythonic language (`.cytho`), implemented in C#. The lexer emits a sequence of `Token` records containing `type`, `lexeme` (normalized lowercase for identifiers/keywords), `raw`, `line`, and `column`. Keyword recognition, identifiers, numbers, operators, and delimiters are all driven by explicit character-level DFAs—no full-string comparisons or string-key maps are used.

## What this project gives you
- A **tokenizer (lexer)** that reads `.cytho` source text and breaks it into meaningful tokens.
- A **command-line tool** (`Cythonic.Cli`) that lets you run `dotnet run --project tools/Cythonic.Cli -- run <file>` so you can see the token list for any `.cytho` file.
- A **comprehensive test suite** that proves the lexer understands keywords, identifiers, numbers, strings, comments, and operators exactly as described in the language spec.
- A **sample program** (`samples/sample.cytho`) plus documentation showing the exact token stream the lexer produces.
- A **state-table description** of the keyword DFA so you can verify or extend the keyword recognition logic without reading the code.

If you are new to lexers, think of this repository as the “front door” of the language pipeline: it reads raw text and turns it into structured pieces that later stages (parser, semantic analyser, compiler) can understand.

## Build & Test
- Install .NET SDK 8.0 or later.
- Restore and build (first run only): `dotnet restore Cythonic.sln`
- Run the full test suite: `dotnet test Cythonic.sln`
- Tokenise a `.cytho` file with the CLI: `dotnet run --project tools/Cythonic.Cli -- run samples/sample.cytho`
  - Outputs JSON token stream to console
  - Generates symbol table file: `<filename>.symboltable.txt`
- `tools/Cythonic.Cli` – command-line entry point that emits JSON token streams and symbol table files.
- `tools/KeywordTrieInspector` – diagnostic tool that dumps the keyword DFA state table.

## How to try it out
1. Open a terminal in the repository root (`Cythonic`).
2. Run `dotnet run --project tools/Cythonic.Cli -- run samples/sample.cytho`.
3. The program prints a neatly formatted JSON array where each entry shows the token type, the normalized lexeme, its original text, and the exact line/column where it appeared.
4. A symbol table file `sample.symboltable.txt` is automatically generated with a formatted table of all tokens.
5. Edit or create your own `.cytho` file and point the same command at it to see how the lexer interprets your code.

## Sample Tokenization
Input (`samples/sample.cytho`):

```cytho
pub class MyClass {
		num COUNT = 10;
		str name = "Earl\n";
		// increment
		COUNT++;
		if (COUNT >= 11) {
				print("ok");
		}
		/* multi
line comment */
}
```

Exact output tokens:

```json
[
	{"type":"KEYWORD","lexeme":"pub","line":1,"column":1,"raw":"pub"},
	{"type":"KEYWORD","lexeme":"class","line":1,"column":5,"raw":"class"},
	{"type":"IDENTIFIER","lexeme":"myclass","line":1,"column":11,"raw":"MyClass"},
	{"type":"DELIMITER","lexeme":"{","line":1,"column":19,"raw":"{"},
	{"type":"TYPE","lexeme":"num","line":2,"column":5,"raw":"num"},
	{"type":"IDENTIFIER","lexeme":"count","line":2,"column":9,"raw":"COUNT"},
	{"type":"OPERATOR","lexeme":"=","line":2,"column":15,"raw":"="},
	{"type":"NUMBER","lexeme":"10","line":2,"column":17,"raw":"10"},
	{"type":"DELIMITER","lexeme":";","line":2,"column":19,"raw":";"},
	{"type":"TYPE","lexeme":"str","line":3,"column":5,"raw":"str"},
	{"type":"IDENTIFIER","lexeme":"name","line":3,"column":9,"raw":"name"},
	{"type":"OPERATOR","lexeme":"=","line":3,"column":14,"raw":"="},
	{"type":"STRING_LITERAL","lexeme":"Earl\n","line":3,"column":16,"raw":"\"Earl\\n\""},
	{"type":"DELIMITER","lexeme":";","line":3,"column":23,"raw":";"},
	{"type":"COMMENT","lexeme":" increment","line":4,"column":5,"raw":"// increment"},
	{"type":"IDENTIFIER","lexeme":"count","line":5,"column":5,"raw":"COUNT"},
	{"type":"OPERATOR","lexeme":"++","line":5,"column":10,"raw":"++"},
	{"type":"DELIMITER","lexeme":";","line":5,"column":12,"raw":";"},
	{"type":"KEYWORD","lexeme":"if","line":6,"column":5,"raw":"if"},
	{"type":"DELIMITER","lexeme":"(","line":6,"column":8,"raw":"("},
	{"type":"IDENTIFIER","lexeme":"count","line":6,"column":9,"raw":"COUNT"},
	{"type":"OPERATOR","lexeme":">=","line":6,"column":15,"raw":">="},
	{"type":"NUMBER","lexeme":"11","line":6,"column":18,"raw":"11"},
	{"type":"DELIMITER","lexeme":")","line":6,"column":20,"raw":")"},
	{"type":"DELIMITER","lexeme":"{","line":6,"column":22,"raw":"{"},
	{"type":"IDENTIFIER","lexeme":"print","line":7,"column":9,"raw":"print"},
	{"type":"DELIMITER","lexeme":"(","line":7,"column":14,"raw":"("},
	{"type":"STRING_LITERAL","lexeme":"ok","line":7,"column":15,"raw":"\"ok\""},
	{"type":"DELIMITER","lexeme":")","line":7,"column":19,"raw":")"},
	{"type":"DELIMITER","lexeme":";","line":7,"column":20,"raw":";"},
	{"type":"DELIMITER","lexeme":"}","line":8,"column":5,"raw":"}"},
	{"type":"COMMENT","lexeme":" multi\nline comment ","line":9,"column":5,"raw":"/* multi\nline comment */"},
	{"type":"DELIMITER","lexeme":"}","line":10,"column":1,"raw":"}"}
]
```

## Keyword DFA State Table

```
STATE | ACCEPT | TYPE            | TRANSITIONS
--------------------------------------------------------------------------------
    0 |  no   | -               | a->1,b->11,c->22,d->33,e->47,f->54,g->65,i->73,l->82,n->85,o->106,p->108,r->116,s->129,t->143,u->150,v->153,w->160
    1 |  no   | -               | n->2,r->4,s->7
    2 |  no   | -               | d->3
    3 | yes   | KEYWORD         |
    4 |  no   | -               | g->5
    5 |  no   | -               | s->6
    6 | yes   | KEYWORD         |
    7 | yes   | KEYWORD         | y->8
    8 |  no   | -               | n->9
    9 |  no   | -               | c->10
   10 | yes   | KEYWORD         |
   11 |  no   | -               | a->12,o->15,r->18
   12 |  no   | -               | s->13
   13 |  no   | -               | e->14
   14 | yes   | KEYWORD         |
   15 |  no   | -               | o->16
   16 |  no   | -               | l->17
   17 | yes   | TYPE            |
   18 |  no   | -               | e->19
   19 |  no   | -               | a->20
   20 |  no   | -               | k->21
   21 | yes   | KEYWORD         |
   22 |  no   | -               | a->23,h->26,l->29
   23 |  no   | -               | s->24
   24 |  no   | -               | e->25
   25 | yes   | KEYWORD         |
   26 |  no   | -               | a->27
   27 |  no   | -               | r->28
   28 | yes   | TYPE            |
   29 |  no   | -               | a->30
   30 |  no   | -               | s->31
   31 |  no   | -               | s->32
   32 | yes   | KEYWORD         |
   33 |  no   | -               | e->34,o->40,y->45
   34 |  no   | -               | f->35
   35 |  no   | -               | a->36
   36 |  no   | -               | u->37
   37 |  no   | -               | l->38
   38 |  no   | -               | t->39
   39 | yes   | KEYWORD         |
   40 | yes   | KEYWORD         | u->41
   41 |  no   | -               | b->42
   42 |  no   | -               | l->43
   43 |  no   | -               | e->44
   44 | yes   | TYPE            |
   45 |  no   | -               | n->46
   46 | yes   | KEYWORD         |
   47 |  no   | -               | l->48,n->51
   48 |  no   | -               | s->49
   49 |  no   | -               | e->50
   50 | yes   | KEYWORD         |
   51 |  no   | -               | u->52
   52 |  no   | -               | m->53
   53 | yes   | KEYWORD         |
   54 |  no   | -               | a->55,o->59
   55 |  no   | -               | l->56
   56 |  no   | -               | s->57
   57 |  no   | -               | e->58
   58 | yes   | BOOLEAN_LITERAL |
   59 |  no   | -               | r->60
   60 | yes   | KEYWORD         | e->61
   61 |  no   | -               | a->62
   62 |  no   | -               | c->63
   63 |  no   | -               | h->64
   64 | yes   | KEYWORD         |
   65 |  no   | -               | e->66,l->68
   66 |  no   | -               | t->67
   67 | yes   | KEYWORD         |
   68 |  no   | -               | o->69
   69 |  no   | -               | b->70
   70 |  no   | -               | a->71
   71 |  no   | -               | l->72
   72 | yes   | KEYWORD         |
   73 |  no   | -               | f->74,n->78
   74 | yes   | KEYWORD         | a->75
   75 |  no   | -               | c->76
   76 |  no   | -               | e->77
   77 | yes   | KEYWORD         |
   78 | yes   | KEYWORD         | i->79,t->81
   79 |  no   | -               | t->80
   80 | yes   | KEYWORD         |
   81 | yes   | TYPE            |
   82 |  no   | -               | e->83
   83 |  no   | -               | t->84
   84 | yes   | KEYWORD         |
   85 |  no   | -               | e->86,m->90,n->93,s->97,u->102
   86 |  no   | -               | w->89,x->87
   87 |  no   | -               | t->88
   88 | yes   | KEYWORD         |
   89 | yes   | KEYWORD         |
   90 |  no   | -               | o->91
   91 |  no   | -               | f->92
   92 | yes   | KEYWORD         |
   93 |  no   | -               | u->94
   94 |  no   | -               | l->95
   95 |  no   | -               | l->96
   96 | yes   | KEYWORD         |
   97 |  no   | -               | p->98
   98 |  no   | -               | a->99
   99 |  no   | -               | c->100
  100 |  no   | -               | e->101
  101 | yes   | KEYWORD         |
  102 |  no   | -               | l->103,m->105
  103 |  no   | -               | l->104
  104 | yes   | KEYWORD         |
  105 | yes   | TYPE            |
  106 |  no   | -               | r->107
  107 | yes   | KEYWORD         |
  108 |  no   | -               | r->109,u->114
  109 |  no   | -               | i->110,o->112
  110 |  no   | -               | v->111
  111 | yes   | KEYWORD         |
  112 |  no   | -               | t->113
  113 | yes   | KEYWORD         |
  114 |  no   | -               | b->115
  115 | yes   | KEYWORD         |
  116 |  no   | -               | d->117,e->119
  117 |  no   | -               | o->118
  118 | yes   | KEYWORD         |
  119 |  no   | -               | c->120,q->124,t->125
  120 | yes   | KEYWORD         | o->121
  121 |  no   | -               | r->122
  122 |  no   | -               | d->123
  123 | yes   | KEYWORD         |
  124 | yes   | KEYWORD         |
  125 |  no   | -               | u->126
  126 |  no   | -               | r->127
  127 |  no   | -               | n->128
  128 | yes   | KEYWORD         |
  129 |  no   | -               | e->130,t->132,w->138
  130 |  no   | -               | t->131
  131 | yes   | KEYWORD         |
  132 |  no   | -               | c->133,r->134
  133 | yes   | KEYWORD         |
  134 | yes   | TYPE            | u->135
  135 |  no   | -               | c->136
  136 |  no   | -               | t->137
  137 | yes   | KEYWORD         |
  138 |  no   | -               | i->139
  139 |  no   | -               | t->140
  140 |  no   | -               | c->141
  141 |  no   | -               | h->142
  142 | yes   | KEYWORD         |
  143 |  no   | -               | h->144,r->147
  144 |  no   | -               | i->145
  145 |  no   | -               | s->146
  146 | yes   | KEYWORD         |
  147 |  no   | -               | u->148
  148 |  no   | -               | e->149
  149 | yes   | BOOLEAN_LITERAL |
  150 |  no   | -               | s->151
  151 |  no   | -               | e->152
  152 | yes   | KEYWORD         |
  153 |  no   | -               | a->154,o->157
  154 |  no   | -               | l->155,r->156
  155 | yes   | KEYWORD         |
  156 | yes   | KEYWORD         |
  157 |  no   | -               | i->158
  158 |  no   | -               | d->159
  159 | yes   | TYPE            |
  160 |  no   | -               | h->161
  161 |  no   | -               | i->162
  162 |  no   | -               | l->163
  163 |  no   | -               | e->164
  164 | yes   | KEYWORD         |

SAMPLE TOKENS
   1:  1 KEYWORD          pub          pub
   1:  5 KEYWORD          class        class
   1: 11 IDENTIFIER       myclass      MyClass
   1: 19 DELIMITER        {            {
   2:  5 TYPE             num          num
   2:  9 IDENTIFIER       count        COUNT
   2: 15 OPERATOR         =            =
   2: 17 NUMBER           10           10
   2: 19 DELIMITER        ;            ;
   3:  5 TYPE             str          str
   3:  9 IDENTIFIER       name         name
   3: 14 OPERATOR         =            =
   3: 16 STRING_LITERAL   Earl
        "Earl\n"
   3: 23 DELIMITER        ;            ;
   4:  5 COMMENT           increment   // increment
   5:  5 IDENTIFIER       count        COUNT
   5: 10 OPERATOR         ++           ++
   5: 12 DELIMITER        ;            ;
   6:  5 KEYWORD          if           if
   6:  8 DELIMITER        (            (
   6:  9 IDENTIFIER       count        COUNT
   6: 15 OPERATOR         >=           >=
   6: 18 NUMBER           11           11
   6: 20 DELIMITER        )            )
   6: 22 DELIMITER        {            {
   7:  9 IDENTIFIER       print        print
   7: 14 DELIMITER        (            (
   7: 15 STRING_LITERAL   ok           "ok"
   7: 19 DELIMITER        )            )
   7: 20 DELIMITER        ;            ;
   8:  5 DELIMITER        }            }
   9:  5 COMMENT           multi
line comment  /* multi
line comment */
  10:  1 DELIMITER        }            }
```

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
- Keywords and identifiers are recognised in a single pass; the trie above determines whether a path terminates in `KEYWORD`, `TYPE`, or `BOOLEAN_LITERAL`. Any deviation (digits or `_`) demotes the token to `IDENTIFIER`. Normalised lexemes are truncated to the first 31 characters.
- Source coordinates follow the specification exactly: escape sequences inside string/char literals contribute width 1 to subsequent columns, and newlines inside block comments are collapsed when reporting line numbers.
- Numeric DFA supports integers (`123`), floats with leading or trailing fractions (`0.5`, `.5`, `10.`), and exponents (`1e10`, `1.23e-4`). Trailing letters or underscores (e.g. `1id`) raise a lexical error.
- String literals accept escapes `\n`, `\t`, `\\`, `\"`, `\'`, `\r`, `\b`, `\f`, `\0`. Unterminated strings/chars and illegal escapes produce descriptive `LexerException`s with line/column data.
- Comments (`//` and `/* */`) are tokenized as `COMMENT` tokens; unterminated block comments are reported as errors. The `Lexeme` field contains the comment content without delimiters (normalized to lowercase), and the `Raw` field includes the original `//` or `/* */` markers.
- Operators and delimiters obey longest-match semantics (`>=`, `==`, `++`, `&&`, `||`), while punctuation such as `;`, `,`, `.`, `(`, `)`, `{`, `}`, `[`, `]`, `:`, and `?` emit `DELIMITER` tokens.

## Project Layout
- `src/CythonicLexer` – lexer library (`Lexer`, `Token`, `TokenType`, `KeywordTrie`).
- `tests/CythonicLexer.Tests` – xUnit test suite covering keywords, identifiers, numbers, operators, comments, literals, and the full sample stream.
- `samples/sample.cytho` – canonical sample program used by documentation and automated tests.

# Cythonic Lexer Flow

Here's how the lexer processes source code, step-by-step (using the analogy you requested):

## High-Level Flow (Login Analogy)

```
Source Text Input → Character Stream → Token Recognition → Token List Output
     ↓                    ↓                    ↓                    ↓
  "if (x)"         Scan char by char    Identify patterns    [{KEYWORD,"if"},{DELIMITER,"("},{IDENTIFIER,"x"},{DELIMITER,")"}]
```

## Detailed Processing Flow

### 1. **Initialization** (User Opens Login Page)
```csharp
var lexer = new Lexer("if (COUNT >= 11)");
```
- Store entire source text
- Initialize position trackers: `_index=0`, `_line=1`, `_column=1`
- Load keyword trie (DFA with all keywords pre-built)

### 2. **Main Loop** (User Types in Form Fields)
```
While not at end of source:
    ├─ Skip whitespace (spaces, tabs, newlines)
    ├─ Mark token start position (line, column, index)
    ├─ Read first character → decide token type
    │   ├─ "//" → LexSingleLineComment
    │   ├─ "/*" → LexMultiLineComment
    │   ├─ letter/underscore → LexIdentifierOrKeyword
    │   ├─ digit → LexNumber
    │   ├─ quote → LexStringLiteral or LexCharLiteral
    │   └─ operator/delimiter → LexOperatorOrDelimiter
    └─ Call appropriate lexer method to emit token
```

### 3. **Character Classification** (Field Validation)
```
Current char = 'i'
    ├─ IsLetter? → YES → LexIdentifierOrKeyword
    ├─ IsDigit? → NO
    ├─ Is quote? → NO
    └─ Is operator? → NO
```

### 4. **Token Recognition** (Submit Button Pressed)

#### **Example A: Keyword `if`**
```
Input: "if"
Flow:
    ├─ Start at 'i' (line 1, col 1)
    ├─ Recognize IsIdentifierStart('i') = true
    ├─ Enter LexIdentifierOrKeyword:
    │   ├─ Read 'i' → normalize to 'i' → check keyword trie → state 73
    │   ├─ Read 'f' → normalize to 'f' → check keyword trie → state 74 (ACCEPTING: KEYWORD)
    │   ├─ Next char '(' → not identifier part → STOP
    │   └─ Trie says state 74 is accepting → Token type = KEYWORD
    └─ Emit: Token(KEYWORD, "if", line:1, col:1, raw:"if")
```

#### **Example B: Identifier `COUNT`**
```
Input: "COUNT"
Flow:
    ├─ Start at 'C' (line 6, col 9)
    ├─ Enter LexIdentifierOrKeyword:
    │   ├─ Read 'C' → normalize to 'c' → check trie → state 22
    │   ├─ Read 'O' → normalize to 'o' → check trie → NO MATCH in keyword trie
    │   ├─ Continue reading: 'U','N','T' → build normalized "count"
    │   ├─ Next char ' ' → not identifier part → STOP
    │   └─ No keyword match → Token type = IDENTIFIER
    └─ Emit: Token(IDENTIFIER, "count", line:6, col:9, raw:"COUNT")
```

#### **Example C: Operator `>=`**
```
Input: ">="
Flow:
    ├─ Start at '>' (line 6, col 15)
    ├─ Recognize IsOperatorStart('>') = true
    ├─ Enter LexOperatorOrDelimiter:
    │   ├─ Read '>' → check next char
    │   ├─ Peek ahead: '=' → match multi-char operator ">="
    │   ├─ Consume both characters
    │   └─ Longest match wins
    └─ Emit: Token(OPERATOR, ">=", line:6, col:15, raw:">=")
```

#### **Example D: Number `11`**
```
Input: "11"
Flow:
    ├─ Start at '1' (line 6, col 18)
    ├─ Recognize IsDigit('1') = true
    ├─ Enter LexNumber:
    │   ├─ Read '1' → append to normalized
    │   ├─ Read '1' → append to normalized
    │   ├─ Next char ')' → not digit/dot/exponent → STOP
    │   └─ Validate: has integer digits, no invalid suffix
    └─ Emit: Token(NUMBER, "11", line:6, col:18, raw:"11")
```

#### **Example E: String `"ok"`**
```
Input: "\"ok\""
Flow:
    ├─ Start at '"' (line 7, col 15)
    ├─ Recognize quote → Enter LexStringLiteral:
    │   ├─ Consume opening '"'
    │   ├─ Read 'o' → append to content
    │   ├─ Read 'k' → append to content
    │   ├─ Read closing '"' → STOP
    │   └─ Validate: string terminated correctly
    └─ Emit: Token(STRING_LITERAL, "ok", line:7, col:15, raw:"\"ok\"")
```

### 5. **Position Tracking** (Breadcrumb Trail)
```
Every Advance() call:
    ├─ Read character at _index
    ├─ If '\n' or '\r\n' → _line++, _column = 1
    ├─ Else → _column++
    └─ _index++

Bias adjustments (for escapes/multi-line comments):
    ├─ Escape '\n' → _columnBias++ (2 source chars = 1 token char)
    └─ Comment spanning lines → _lineBias++ (hide internal lines)
```

### 6. **Error Handling** (Access Denied)
```
Invalid cases throw LexerException:
    ├─ Unterminated string: "hello
    ├─ Invalid escape: "\q"
    ├─ Bad number: "1.2.3"
    └─ Unknown character: "@"

Exception contains: error message, line, column
```

### 7. **Final Output** (Login Success → Dashboard)
```csharp
List<Token> tokens = lexer.Lex();
// Returns:
[
    Token(KEYWORD, "if", 1, 1, "if"),
    Token(DELIMITER, "(", 1, 4, "("),
    Token(IDENTIFIER, "count", 1, 5, "COUNT"),
    Token(OPERATOR, ">=", 1, 11, ">="),
    Token(NUMBER, "11", 1, 14, "11"),
    Token(DELIMITER, ")", 1, 16, ")"),
    Token(EOF, "", 1, 17, "")
]
```

## Visual Diagram

```
┌─────────────────────────────────────────────────────────┐
│  SOURCE: "if (COUNT >= 11)"                             │
└────────────┬────────────────────────────────────────────┘
             │
             ▼
┌────────────────────────────────────────────────────────┐
│  LEXER INITIALIZATION                                   │
│  • Load keyword trie (73 → 'i', 74 → 'f' = KEYWORD)    │
│  • Set _index=0, _line=1, _column=1                    │
└────────────┬───────────────────────────────────────────┘
             │
             ▼
┌────────────────────────────────────────────────────────┐
│  MAIN LOOP: while (!IsAtEnd())                         │
└────────────┬───────────────────────────────────────────┘
             │
             ├─► Skip whitespace (spaces, tabs, newlines)
             │
             ├─► Peek first char → classify
             │      │
             │      ├─ "//" or "/*" → LexSingleLineComment / LexMultiLineComment → emit COMMENT token
             │      ├─ Letter/underscore → LexIdentifierOrKeyword
             │      ├─ Digit → LexNumber
             │      ├─ Quote → LexStringLiteral / LexCharLiteral
             │      └─ Operator char → LexOperatorOrDelimiter
             │
             ▼
┌────────────────────────────────────────────────────────┐
│  TOKEN EMISSION                                         │
│  _tokens.Add(new Token(...))                           │
└────────────┬───────────────────────────────────────────┘
             │
             ▼ (repeat until end)
┌────────────────────────────────────────────────────────┐
│  RETURN: IReadOnlyList<Token>                          │
│  [KEYWORD, DELIMITER, IDENTIFIER, OPERATOR, ...]       │
└────────────────────────────────────────────────────────┘
```

## Key Design Principles (Security Best Practices Analogy)

| Lexer Feature | Login Analogy |
|--------------|---------------|
| **DFA keyword recognition** | Password stored as hash (no plaintext comparison) |
| **Position tracking** | Audit log (track every action with timestamp) |
| **Error exceptions** | Failed login attempt with reason + location |
| **Case-insensitive** | Email login (user@DOMAIN = user@domain) |
| **Longest match** | Greedy token grab (don't stop at `>` when `>=` exists) |
| **No string maps** | Direct state machine (no database lookup per token) |

This architecture ensures **O(n) time complexity** with **constant memory per character**, making it production-ready for large source files.