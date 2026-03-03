# Math DSL Compiler

A custom Domain Specific Language (DSL) compiler built in C. It parses mathematical equations and variable assignments, reconstructing them into an Abstract Syntax Tree (AST) that enforces mathematical order-of-operations (BEDMAS/PEMDAS). It also visually outputs the generated AST.

## Features

- **Lexical Analysis:** Custom lexer that converts raw source text into tokens, tracking exact line and column numbers for precise error reporting.
- **Abstract Syntax Tree (AST):** Dynamically allocates a representation of expressions using union-based nodes for assignments, operations, variables, and numbers.
- **Syntax Analysis:** A recursive descent parser that enforces rigorous operator precedence (multiplication/division evaluated before addition/subtraction).
- **Panic-Mode Error Recovery:** Resilient parser that gracefully recovers from syntax errors to continue evaluating the remaining source text.
- **AST Visualization:** Automatically generates a textual `.dot` file representation of the AST and calls Graphviz to compile it into a high-quality `.png` image.

## Project Structure

- `src/main.c`: The compiler entry point tying everything together.
- `src/lexer.c`: Lexical analyzer to tokenize the input.
- `src/parser.c`: Recursive descent parser to build the AST.
- `src/ast.c`: Definitions and memory management functionality for AST nodes.
- `src/visualize.c`: AST walker that outputs Graphviz DOT structure.
- `include/`: C header files for the implementation.
- `project_report.md`: Detailed architecture report with insights into the logic and parsing strategies.

## Prerequisites

- **CMake** (3.15 or newer)
- **C Compiler** supporting C99 standard
- **Graphviz** (must be installed and added to the system `PATH` to allow dynamic `.png` generation)

## Building the Compiler

1. Create a `build` directory and run CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
2. Build the executable:
   ```bash
   cmake --build .
   ```
   *(Or open the generated files in your preferred IDE/Make to compile).*

## Running the Compiler

To use the compiler, pass the path of a source text file containing equations/assignments:

```bash
./math_dsl <path_to_source_file.txt>
```

Example source text:
```
let x = 10 + 5 * -2;
```

**Output:**
1. Console will print `Successfully parsed...` and status updates.
2. An `ast.dot` file will be generated in the current directory.
3. If Graphviz is correctly installed, an `ast.png` file will be generated containing the visual tree.

**Note:** If the `dot` command fails, please ensure that Graphviz is properly installed and its `bin` directory is added to your system's `PATH` variable.
