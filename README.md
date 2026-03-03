# Math-DSL Compiler

A high-performance, handwritten Domain Specific Language (DSL) compiler developed in **Pure C**. This project implements a full compilation pipeline—from lexical analysis to AST visualization—without the use of third-party generators like Flex or Bison. It is specifically designed to handle mathematical expressions and variable assignments while strictly enforcing operator precedence.

## 🛠 Technical Features

* **Handwritten Lexical Analyzer:** A custom state machine that tokenizes input and tracks precise line/column metadata for advanced error reporting.
* **Recursive Descent Parser:** Manually implemented parsing logic to handle complex operator precedence (PEMDAS/BEDMAS) and nested parentheses.
* **Dynamic AST Construction:** An Abstract Syntax Tree built using union-based nodes and manual memory management for high efficiency.
* **Panic-Mode Error Recovery:** Resilient architecture that identifies syntax errors without halting the entire compilation process.
* **Graphviz Integration:** Automated generation of `.dot` files and `.png` images to provide a visual representation of the compiler's internal logic.

## 🏗 System Architecture

The project follows a modular C architecture to ensure scalability and maintainability:

| Component | File | Description |
| --- | --- | --- |
| **Lexer** | `lexer.c` | Converts raw source text into a stream of tokens. |
| **Parser** | `parser.c` | Performs syntax analysis and constructs the AST. |
| **AST** | `ast.c` | Handles node allocation, tree traversal, and memory cleanup. |
| **Visualizer** | `visualize.c` | Walks the AST to generate Graphviz-compatible output. |
| **Driver** | `main.c` | The main entry point coordinating the compilation phases. |

## 🚀 Getting Started

### Prerequisites

* **C Compiler:** GCC or Clang (C11 support)
* **Build System:** CMake 3.15+
* **Visualization:** Graphviz (must be in system `PATH` for `.png` generation)

### Build Instructions

```powershell
# Create build directory
mkdir build
cd build

# Generate build files and compile
cmake ..
cmake --build .

```

### Usage

Run the compiler by passing a source file:

```bash
./math_dsl tests/sample.mdsl

```

## 📊 Sample Output

Input: `let x = 10 + 5 * 2;`

The compiler generates an **`ast.png`** representing the prioritized operations:

---
