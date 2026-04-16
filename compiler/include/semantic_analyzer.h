#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <vector>

// ── Semantic Error ────────────────────────────────────────────────────────────
struct SemanticError {
  std::string message;
  int         line   = 0;
  int         column = 0;
};

// ── Semantic Analyzer — Visitor over the ASTNode variant ─────────────────────
//
// Pass errors() to the IDE error-marker layer after calling analyze().
//
class SemanticAnalyzer {
public:
  SemanticAnalyzer();

  // Returns true if no errors were found.
  bool analyze(const ASTNode& root);

  const std::vector<SemanticError>& errors() const { return errors_; }
  bool hadError() const { return !errors_.empty(); }

private:
  SymbolTable               table_;
  std::vector<SemanticError> errors_;

  // ── Dispatch ─────────────────────────────────────────────────────────────
  ValueType visit(const ASTNode& node);

  // ── Per-node visitors ─────────────────────────────────────────────────────
  ValueType visitNumber    (const NumberNode&);
  ValueType visitVariable  (const VariableNode&);
  ValueType visitBinary    (const BinaryNode&);
  ValueType visitUnary     (const UnaryNode&);
  ValueType visitAssignment(const AssignmentNode&);
  ValueType visitCall      (const CallNode&);
  ValueType visitBlock     (const BlockNode&);
  ValueType visitIf        (const IfNode&);
  ValueType visitWhile     (const WhileNode&);
  ValueType visitProgram   (const ProgramNode&);

  // ── Helpers ───────────────────────────────────────────────────────────────
  void reportError(const std::string& msg, int line, int column = 0);

  // Built-in function arity table
  static int builtinArity(const std::string& name); // -1 = not built-in
};
