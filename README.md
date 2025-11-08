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
- Tokenise a `.cytho` file with the CLI: `dotnet run --project tools/Cythonic.Cli -- run <path>`
- `tools/Cythonic.Cli` – command-line entry point (`cythonic run path`) that emits JSON token streams.

## How to try it out
1. Open a terminal in the repository root (`Cythonic`).
2. Run `dotnet run --project tools/Cythonic.Cli -- run samples/sample.cytho`.
3. The program prints a neatly formatted JSON array where each entry shows the token type, the normalized lexeme, its original text, and the exact line/column where it appeared.
4. Edit or create your own `.cytho` file and point the same command at it to see how the lexer interprets your code.

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
- Comments (`//` and `/* */`) are treated as whitespace; unterminated block comments are reported as errors.
- Operators and delimiters obey longest-match semantics (`>=`, `==`, `++`, `&&`, `||`), while punctuation such as `;`, `,`, `.`, `(`, `)`, `{`, `}`, `[`, `]`, `:`, and `?` emit `DELIMITER` tokens.

## Project Layout
- `src/CythonicLexer` – lexer library (`Lexer`, `Token`, `TokenType`, `KeywordTrie`).
- `tests/CythonicLexer.Tests` – xUnit test suite covering keywords, identifiers, numbers, operators, comments, literals, and the full sample stream.
- `samples/sample.cytho` – canonical sample program used by documentation and automated tests.
