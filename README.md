# 🧮 Math DSL Compiler

A fully-featured **Domain-Specific Language (DSL) compiler** for mathematical expressions, built in **modern C++17**. It compiles `.dsl` source files through a complete compiler pipeline: Lexing → Parsing → Semantic Analysis → IR Generation → Optimization, with both a **CLI tool** and an **ImGui-based graphical IDE**.

---

## ✨ Features

- ✅ Full **lexer + recursive-descent parser** with proper operator precedence
- ✅ **Abstract Syntax Tree (AST)** construction and visualization (Graphviz `.dot` → `.png`)
- ✅ **Semantic analysis** — variable scoping, type checking, undeclared variable detection
- ✅ **Symbol table** with nested scope support (block scoping, shadowing)
- ✅ **Intermediate Representation (IR)** generation
- ✅ **Optimizer** with constant folding, algebraic simplification, and dead code elimination
- ✅ **Interpreter** for direct expression evaluation
- ✅ **CLI tool** for scripting and automation
- ✅ **ImGui-based GUI IDE** with syntax highlighting and live output

---

## 🗂️ Project Structure

```
Math-dsl-compiler/
├── CMakeLists.txt              # Root build configuration
├── compiler/                   # Core compiler library (static)
│   ├── include/
│   │   ├── common.h            # Token & AST node type enums
│   │   ├── lexer.h             # Tokenizer declarations
│   │   ├── parser.h            # Recursive-descent parser
│   │   ├── ast.h               # AST node definitions
│   │   ├── ast_printer.h       # Pretty-print the AST
│   │   ├── semantic_analyzer.h # Scope & type checking
│   │   ├── symbol_table.h      # Scoped symbol table
│   │   ├── interpreter.h       # Tree-walk interpreter
│   │   ├── ir.h                # IR instruction definitions
│   │   ├── ir_gen.h            # IR code generator
│   │   ├── optimizer.h         # Optimization passes
│   │   └── visualize.h         # Graphviz DOT output
│   └── src/
│       ├── lexer.cpp
│       ├── parser.cpp
│       ├── ast.cpp
│       ├── ast_printer.cpp
│       ├── semantic_analyzer.cpp
│       ├── interpreter.cpp
│       ├── ir_gen.cpp
│       ├── optimizer.cpp
│       └── visualize.cpp
├── cli/                        # Command-line frontend
│   ├── CMakeLists.txt
│   └── main.cpp
├── gui/                        # ImGui IDE frontend
│   ├── CMakeLists.txt
│   ├── main_gui.cpp
│   └── vendor/
│       ├── imgui/              # Dear ImGui library
│       ├── ImGuiColorTextEdit/ # Syntax-highlighted editor
│       └── stb_image.h
├── tests/                      # Sample DSL source files
│   ├── test.dsl
│   ├── test_errors.dsl
│   ├── test_power.dsl
│   └── sample.mdsl
└── .gitignore
```

---

## 🧠 Compiler Pipeline

```
 Source (.dsl file)
        │
        ▼
   ┌─────────┐
   │  Lexer  │  ──► Tokenizes source into Token stream
   └────┬────┘
        │
        ▼
   ┌─────────┐
   │ Parser  │  ──► Builds Abstract Syntax Tree (AST)
   └────┬────┘
        │
        ▼
   ┌──────────────────┐
   │ Semantic Analyzer│  ──► Scope/type checks, symbol table
   └────────┬─────────┘
            │
            ▼
   ┌─────────────────┐
   │  AST Visualizer │  ──► Outputs ast.dot + ast.png (via Graphviz)
   └────────┬────────┘
            │
            ▼
   ┌──────────┐
   │  IR Gen  │  ──► Generates flat Intermediate Representation
   └────┬─────┘
        │
        ▼
   ┌───────────┐
   │ Optimizer │  ──► Constant folding, dead code elimination
   └────┬──────┘
        │
        ▼
   Final IR Output (printed to stdout)
```

---

## 📝 DSL Language Syntax

### Variables
```dsl
let x = 10;
let y = (x * 2) + 5;
```

### Arithmetic Operators
```dsl
let a = 3 + 4;     // Addition
let b = 10 - 2;    // Subtraction
let c = 6 * 7;     // Multiplication
let d = 15 / 3;    // Division
let e = 2 ^ 8;     // Exponentiation (right-associative)
```

### Comparison & Logical Operators
```dsl
let eq  = (x == y);
let neq = (x != y);
let lt  = (x < y);
let lte = (x <= y);
let gt  = (x > y);
let gte = (x >= y);
let neg = !false;
```

### Built-in Functions & Constants
```dsl
let s = sin(0);
let r = sqrt(16 + 9);
let p = 2 ^ 3 ^ 2;    // Right-assoc: 2^(3^2) = 512
```

### Block Scoping
```dsl
let a = 100;
{
    let a = 50;      // Shadows outer 'a'
    let b = a + 10;  // Uses 50
}
// b is out of scope here, a is back to 100
```

### Control Flow
```dsl
let count = 0;
let limit = 5;

while (count < limit) {
    if (count == 3) {
        let special = 1;
    } else {
        let special = 0;
    }
    count = count + 1;
}
```

### Comments
```dsl
// This is a single-line comment
let x = 42; // Inline comment
```

---

## 🔧 Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| **CMake** | ≥ 3.16 | Build system |
| **C++ Compiler** | C++17 support | `g++`, `clang++`, or MSVC |
| **Graphviz** *(optional)* | Any | Auto-generate `ast.png` from `ast.dot` |
| **OpenGL + SDL2** *(optional)* | — | Required only to build the GUI |

### Installing Prerequisites on Windows

**CMake:**
```powershell
winget install Kitware.CMake
```

**MinGW (GCC for Windows):**
```powershell
winget install MSYS2.MSYS2
# Then inside MSYS2: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
```

**Graphviz (optional, for AST image):**
```powershell
winget install Graphviz.Graphviz
```

---

## 🚀 Building the Project

### Step 1 — Clone the Repository
```bash
git clone https://github.com/Aryan00Saini/Math-dsl-compiler.git
cd Math-dsl-compiler
```

### Step 2 — Configure with CMake

**Build CLI only (recommended — no extra dependencies):**
```bash
cmake -S . -B build -DBUILD_CLI=ON -DBUILD_GUI=OFF
```

**Build both CLI + GUI:**
```bash
cmake -S . -B build -DBUILD_CLI=ON -DBUILD_GUI=ON
```

> On Windows with Visual Studio:
> ```powershell
> cmake -S . -B build -G "Visual Studio 17 2022" -DBUILD_CLI=ON -DBUILD_GUI=OFF
> ```

### Step 3 — Compile
```bash
cmake --build build --config Release
```

The compiled binaries will be in:
- **CLI:** `build/cli/math_dsl` (or `math_dsl.exe` on Windows)
- **GUI:** `build/gui/math_dsl_gui` (or `math_dsl_gui.exe` on Windows)

---

## ▶️ Running the CLI Compiler

```bash
# Linux / macOS
./build/cli/math_dsl <path-to-file.dsl>

# Windows (PowerShell)
.\build\cli\Release\math_dsl.exe <path-to-file.dsl>
```

### Example — Run the provided test file:
```bash
./build/cli/math_dsl tests/test.dsl
```

**Expected output:**
```
✓ Parse OK
✓ Semantic analysis OK
✓ Generated 'ast.dot'
✓ Generated 'ast.png'        ← only if Graphviz is installed
✓ IR generated (42 instructions)
✓ Optimized: 42 → 38 instructions  (4 eliminated)
[IR output printed here...]
```

### Example — Run the error test:
```bash
./build/cli/math_dsl tests/test_errors.dsl
```

### Example — Run power/exponentiation test:
```bash
./build/cli/math_dsl tests/test_power.dsl
```

---

## 🖥️ Running the GUI IDE

```bash
# Linux / macOS
./build/gui/math_dsl_gui

# Windows
.\build\gui\Release\math_dsl_gui.exe
```

The GUI provides:
- A **syntax-highlighted code editor** (powered by ImGuiColorTextEdit)
- **Live compile** output panel
- **AST visualization** panel
- **IR output** viewer

---

## 📄 Writing Your Own `.dsl` File

1. Create a new file, e.g. `myprogram.dsl`
2. Write your Math DSL code (see syntax section above)
3. Run it:
   ```bash
   ./build/cli/math_dsl myprogram.dsl
   ```
4. Check the generated `ast.dot` / `ast.png` for the AST visualization

---

## 🔍 Optimizer Passes

The optimizer runs three passes on the generated IR:

| Pass | Description | Example |
|------|-------------|---------|
| **Constant Folding** | Evaluates constant expressions at compile time | `10 + 20` → `30` |
| **Algebraic Simplification** | Simplifies algebraic identities | `x * 1` → `x`, `x + 0` → `x` |
| **Dead Code Elimination** | Removes instructions whose results are never used | Unused temporaries removed |

---

## 🐛 Error Handling

The compiler reports errors with descriptive messages:

```
[Line 3] Error: Undeclared variable 'z'.
[Line 7] Error: Division by zero detected.

Semantic analysis failed with 2 error(s).
```

Even on error, the AST is still visualized so you can inspect the parse tree.

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "Add my feature"`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a Pull Request

---


## 📜 License

This project is open-source. Feel free to use and modify it for educational purposes.
