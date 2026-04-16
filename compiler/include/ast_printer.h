#pragma once
#include "ast.h"
#include <string>

// ── AST Pretty-Printer ────────────────────────────────────────────────────────
//
// Produces a readable indented tree, e.g.:
//
//   Program
//   ├── let x = ...
//   │   └── Number(10)
//   ├── let y = ...
//   │   └── BinaryOp(+)
//   │       ├── BinaryOp(*)
//   │       │   ├── Variable(x)
//   │       │   └── Number(2)
//   │       └── Number(5)
//   └── while
//       ├── [cond]  BinaryOp(<)
//       └── [body]  Block
//
std::string prettyPrintAST(const ASTNode& root);
