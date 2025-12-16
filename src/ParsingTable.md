# Parsing Table (LL(1))

This table represents the predictive parsing logic implemented in the Recursive Descent Parser.

| Non-Terminal | `TYPE` | `var`/`const`/`dyn` | `IDENTIFIER` | `input` | `print` | `while` | `for` | `if` | `return` | `{` |
|--------------|--------|---------------------|--------------|---------|---------|---------|-------|------|----------|-----|
| **Statement** | Declaration | Declaration | Assignment / Call | Input | Output | While | For | If | Return | Block |
| **Declaration**| `TYPE ID ...` | `var ID ...` | | | | | | | | |
| **Assignment** | | | `ID Op Expr ;` | | | | | | | |
| **Input** | | | | `input(...)` | | | | | | |
| **Output** | | | | | `print(...)` | | | | | |
| **While** | | | | | | `while(...)` | | | | |
| **For** | | | | | | | `for(...)` | | | |
| **If** | | | | | | | | `if(...)` | | |
| **Return** | | | | | | | | | `return ...` | |
| **Block** | | | | | | | | | | `{ ... }` |

## Grammar Productions

1.  **Program** -> `Statement`*
2.  **Statement** -> `Declaration` | `Assignment` | `Input` | `Output` | `While` | `For` | `If` | `Return` | `Block`
3.  **Declaration** -> (`TYPE` | `var` | `const` | `dyn`) `IDENTIFIER` (`=` `Expression`)? `;`
4.  **Assignment** -> `IDENTIFIER` (`=` | `+=` | `-=` | `*=` | `/=` | `%=`) `Expression` `;`
5.  **Input** -> `input` `(` `IDENTIFIER` `)` `;`
6.  **Output** -> `print` `(` `Expression` `)` `;`
7.  **If** -> `if` `(` `Expression` `)` `Statement` (`else` `Statement`)?
8.  **While** -> `while` `(` `Expression` `)` `Statement`
9.  **For** -> `for` `(` (`Declaration` | `Assignment` | `;`) `Expression`? `;` `Expression`? `)` `Statement`
10. **Return** -> `return` `Expression`? `;`
11. **Block** -> `{` `Statement`* `}`
