# Cythonic Compiler/Interpreter Explanation

This document provides a technical walkthrough of the **Cythonic** language implementation, explaining how the source code is processed from raw text to execution.

## 1. System Architecture

The Cythonic compiler follows a standard single-pass interpreter architecture implemented in C:

1.  **Lexical Analysis (Scanning)**: The source code (`.cytho`) is read character-by-character and grouped into meaningful **Tokens**. These tokens are stored in a **Symbol Table**.
2.  **Syntax Analysis (Parsing)**: A **Recursive Descent Parser** consumes the tokens to verify the grammatical structure of the program. It generates an implicit **Parse Tree** (traced in the output file).
3.  **Semantic Analysis & Execution**: As the parser validates the syntax, it simultaneously **executes** the code (Interpreter pattern). It uses an **Environment** to store variable states and executes logic instructions immediately.

## 2. Walkthrough: From Code to Execution

We will analyze the processing of `sample.cytho` (using the clean subset `sample.cytho`).

### A. Lexical Analysis (The Symbol Table)

The first step is converting code into tokens. The scanner identifies Keywords (`var`, `if`), Types (`int`, `str`), Identifiers (`x`, `name`), and Literals (`10`, `"Hello"`).

**Source Code:**
```c
var int x;
```

**Generated Symbol Table (`.symboltable.txt`):**
```text
LINE | COL | TYPE              | LEXEME
-----|-----|-------------------|---------
   8 |   1 | KEYWORD           | var
   8 |   5 | TYPE              | int
   8 |   9 | IDENTIFIER        | x
   8 |  10 | SEMICOLON         | ;
```

*   **Logic**: The scanner skips whitespace and comments (`//...`). When it sees `var`, it matches it against reserved keywords. `x` is recognized as an identifier because it doesn't match a keyword.

### B. Syntax Analysis (The Parse Tree)

The parser expects a specific sequence of tokens defined by the language grammar. For a declaration, it expects: `KEYWORD(var)` -> `TYPE` -> `IDENTIFIER` -> `SEMICOLON`.

**Parse Tree Trace (`.parsetree.txt`):**
```text
Enter <Statement>
  Next token is: KEYWORD Next lexeme is var
  Enter <DeclarationStatement>
    Next token is: TYPE Next lexeme is int
    Next token is: IDENTIFIER Next lexeme is x
    Next token is: SEMICOLON Next lexeme is ;
  Exit <DeclarationStatement>
Exit <Statement>
```

*   **Logic**: The function `statement()` calls `declaration_statement()`, which consumes the expected tokens. If a token is missing (e.g., missing `;`), `consume()` executes an error routine.

### C. Execution (The Interpreter)

Since this is an interpreter, the code executes *during* parsing.

#### 1. Variable Storage (The Environment)
When `var int x;` is parsed:
1.  `declaration_statement` runs.
2.  It calls `env_define(&parser->env, "x", value, false)`.
3.  A new node is added to the linked list `Env`, storing `"x"` and its initial value (default 0).

#### 2. Assignments
Code: `x = 10;`
1.  Parser identifies `IDENTIFIER(x)` followed by `EQUAL`.
2.  `assignment_statement` calls `expression` to evaluate the right-hand side (`10`).
3.  `env_assign` updates the value of `"x"` in the Environment to `10`.

#### 3. Control Flow (If/Else)
Code:
```c
if (x > 5) {
    print("x is greater than 5");
}
```
1.  **Evaluation**: The expression `x > 5` is evaluated. `x` is fetched from Env (10). `10 > 5` is `True`.
2.  **Branching**: `if_statement` checks `if (condition)`. Since it is true, it calls `statement()` (the block with print).
3.  **Result**: The interpreter prints `x is greater than 5`.

#### 4. Iteration (Loops)
Code:
```c
while (count < 5) { ... count++; }
```
The interpreter handles loops using a custom `jump_to(token_index)` function:
1.  **Mark**: The parser records the `token_index` of the condition (`count < 5`).
2.  **Check**: It evaluates the condition. If true, it executes the body.
3.  **Loop**: At the end of the body, `jump_to` rewinds the `parser->current_index` back to the condition.
4.  **Repeat**: This cycle continues until the condition evaluates to false.

#### 5. Input/Output
Code: `input(name);` and `print(sum);`
*   **Input**: Uses C's `scanf` to read a value from standard input and updates the variable in `Env`.
    *   *Note*: The current implementation primarily supports integer input via `scanf("%d")`.
*   **Output**: Uses C's `printf` to display values. It checks the type tag (`VAL_INT`, `VAL_STRING`) to format the output correctly.

## 3. Sample Output Analysis

Based on the execution of `samples/sample.cytho`:

```text
Enter value for name: A          <-- Logic: input(name) reads input (simulated)
Hello from Cythonic!             <-- Logic: print(message)
45                               <-- Logic: sum = x + y + z (10 + 10 + 25)
x is greater than 5              <-- Logic: if (x > 5) where x=10
0                                <-- Logic: while (count < 5) loop output
1
2...
```

The system successfully demonstrates the full pipeline:
**Source Text** -> **Tokens** -> **Grammar Validation** -> **Semantic Action** -> **Output**.