#pragma once
#include "ast.h"
#include <string>
#include <unordered_map>
#include <vector>

// ── Interpreter Result ────────────────────────────────────────────────────────
struct ExecResult {
  // Variables visible in the outermost (global) scope at end of execution
  // Stored in declaration order for display
  struct VarEntry {
    std::string name;
    double      value = 0.0;
  };
  std::vector<VarEntry> vars;

  // Lines emitted by print() calls (future)
  std::vector<std::string> printLines;

  // Runtime errors (e.g. division by zero, sqrt of negative)
  std::vector<std::string> errors;

  bool hadError() const { return !errors.empty(); }
};

// ── Tree-Walking Interpreter ──────────────────────────────────────────────────
//
// Evaluates the AST directly. Uses a scope stack of env maps.
// Built-in math functions forward to <cmath>.
// While loops are limited to MAX_LOOP_ITERS to prevent hangs.
//
class Interpreter {
public:
  static constexpr int MAX_LOOP_ITERS = 100'000;

  ExecResult run(const ASTNode& root);

private:
  // A scope is a map of name → value
  // scopes_[0] = global, back() = innermost
  std::vector<std::unordered_map<std::string, double>> scopes_;
  // Declaration order tracking (global scope only)
  std::vector<std::string> globalOrder_;

  ExecResult* result_ = nullptr;  // non-owning ptr to current result

  // ── Evaluation (returns a double value) ──────────────────────────────────
  double eval    (const ASTNode& node);
  double evalNum (const NumberNode&);
  double evalVar (const VariableNode&);
  double evalBin (const BinaryNode&);
  double evalUna (const UnaryNode&);
  double evalAsgn(const AssignmentNode&);
  double evalCall(const CallNode&);

  // ── Execution (for statements that produce no value) ─────────────────────
  void execBlock  (const BlockNode&);
  void execIf     (const IfNode&);
  void execWhile  (const WhileNode&);
  void execProgram(const ProgramNode&);

  // ── Environment helpers ───────────────────────────────────────────────────
  void   set(const std::string& name, double value, bool isDecl);
  double get(const std::string& name, int line);

  void runtimeError(const std::string& msg);
};
