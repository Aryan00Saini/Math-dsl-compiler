#include "semantic_analyzer.h"
#include <iostream>
#include <unordered_set>
#include <unordered_map>

SemanticAnalyzer::SemanticAnalyzer() = default;

bool SemanticAnalyzer::analyze(const ASTNode& root) {
  errors_.clear();
  visit(root);
  return errors_.empty();
}

void SemanticAnalyzer::reportError(const std::string& msg, int line, int column) {
  std::cerr << "[Semantic][line " << line << "] " << msg << "\n";
  errors_.push_back({msg, line, column});
}

// ── Built-in arity table ─────────────────────────────────────────────────────
// Returns expected argument count, or -1 if not a built-in.
int SemanticAnalyzer::builtinArity(const std::string& name) {
  static const std::unordered_map<std::string, int> table = {
    {"sin",   1}, {"cos",   1}, {"tan",   1},
    {"asin",  1}, {"acos",  1}, {"atan",  1},
    {"sqrt",  1}, {"log",   1}, {"log2",  1}, {"log10", 1},
    {"floor", 1}, {"ceil",  1}, {"abs",   1}, {"exp",   1},
    {"atan2", 2}, {"pow",   2}, {"max",   2}, {"min",   2},
  };
  auto it = table.find(name);
  return it != table.end() ? it->second : -1;
}

// ── Central dispatch ─────────────────────────────────────────────────────────
ValueType SemanticAnalyzer::visit(const ASTNode& node) {
  return std::visit([&](auto& n) -> ValueType {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, NumberNode>)     return visitNumber(n);
    if constexpr (std::is_same_v<T, VariableNode>)   return visitVariable(n);
    if constexpr (std::is_same_v<T, BinaryNode>)     return visitBinary(n);
    if constexpr (std::is_same_v<T, UnaryNode>)      return visitUnary(n);
    if constexpr (std::is_same_v<T, AssignmentNode>) return visitAssignment(n);
    if constexpr (std::is_same_v<T, CallNode>)       return visitCall(n);
    if constexpr (std::is_same_v<T, BlockNode>)      return visitBlock(n);
    if constexpr (std::is_same_v<T, IfNode>)         return visitIf(n);
    if constexpr (std::is_same_v<T, WhileNode>)      return visitWhile(n);
    if constexpr (std::is_same_v<T, ProgramNode>)    return visitProgram(n);
    return ValueType::UNKNOWN;
  }, node.data);
}

// ── Per-node visitors ────────────────────────────────────────────────────────

ValueType SemanticAnalyzer::visitNumber(const NumberNode&) {
  return ValueType::NUMBER;
}

ValueType SemanticAnalyzer::visitVariable(const VariableNode& node) {
  auto sym = table_.lookup(node.name);
  if (!sym) {
    reportError("Variable '" + node.name + "' is not defined.", node.line, node.column);
    return ValueType::UNKNOWN;
  }
  if (!sym->defined) {
    reportError("Variable '" + node.name + "' is used before being assigned.",
                node.line, node.column);
  }
  return sym->type;
}

ValueType SemanticAnalyzer::visitBinary(const BinaryNode& node) {
  ValueType lt = visit(*node.left);
  ValueType rt = visit(*node.right);

  // Arithmetic operators: both sides must be NUMBER
  bool isArith = (node.op == TokenType::PLUS  || node.op == TokenType::MINUS ||
                  node.op == TokenType::STAR  || node.op == TokenType::SLASH ||
                  node.op == TokenType::CARET);

  if (isArith) {
    if (lt == ValueType::BOOLEAN)
      reportError("Cannot use boolean in arithmetic expression (left operand).", 0);
    if (rt == ValueType::BOOLEAN)
      reportError("Cannot use boolean in arithmetic expression (right operand).", 0);
    return ValueType::NUMBER;
  }

  // Comparison operators: produce BOOLEAN
  bool isCmp = (node.op == TokenType::LESS         || node.op == TokenType::LESS_EQUAL  ||
                node.op == TokenType::GREATER       || node.op == TokenType::GREATER_EQUAL ||
                node.op == TokenType::EQUAL_EQUAL   || node.op == TokenType::BANG_EQUAL);
  if (isCmp) return ValueType::BOOLEAN;

  return ValueType::UNKNOWN;
}

ValueType SemanticAnalyzer::visitUnary(const UnaryNode& node) {
  ValueType rt = visit(*node.right);

  if (node.op == TokenType::BANG) {
    // '!' works on booleans and numbers (truthy/falsy)
    return ValueType::BOOLEAN;
  }
  if (node.op == TokenType::MINUS) {
    if (rt == ValueType::BOOLEAN)
      reportError("Cannot negate a boolean value.", 0);
    return ValueType::NUMBER;
  }
  return ValueType::UNKNOWN;
}

ValueType SemanticAnalyzer::visitAssignment(const AssignmentNode& node) {
  ValueType vt = visit(*node.value);

  if (node.isDecl) {
    // 'let x = ...' — declare in current scope
    if (!table_.declare(node.name, vt, node.line)) {
      reportError("Variable '" + node.name + "' is already declared in this scope.",
                  node.line);
    }
    table_.define(node.name);
  } else {
    // bare 'x = ...' — look up and update
    auto sym = table_.lookup(node.name);
    if (!sym) {
      reportError("Assignment to undeclared variable '" + node.name +
                  "'. Use 'let' to declare it.", node.line);
    } else {
      table_.define(node.name);
      table_.setType(node.name, vt);
    }
  }
  return vt;
}

ValueType SemanticAnalyzer::visitCall(const CallNode& node) {
  // Check all arguments
  for (auto& arg : node.args)
    visit(*arg);

  int arity = builtinArity(node.callee);
  if (arity != -1) {
    // Built-in function — validate arity
    if (static_cast<int>(node.args.size()) != arity) {
      reportError("Built-in '" + node.callee + "' expects " +
                  std::to_string(arity) + " argument(s), got " +
                  std::to_string(node.args.size()) + ".", node.line);
    }
    return ValueType::NUMBER;
  }

  // User-defined function — look up symbol
  auto sym = table_.lookup(node.callee);
  if (!sym) {
    reportError("Undefined function '" + node.callee + "'.", node.line);
    return ValueType::UNKNOWN;
  }
  if (sym->type != ValueType::FUNCTION) {
    reportError("'" + node.callee + "' is not a function.", node.line);
  }
  return ValueType::NUMBER;
}

ValueType SemanticAnalyzer::visitBlock(const BlockNode& node) {
  table_.enterScope();
  for (auto& stmt : node.statements)
    visit(*stmt);
  table_.exitScope();
  return ValueType::UNKNOWN;
}

ValueType SemanticAnalyzer::visitIf(const IfNode& node) {
  ValueType condType = visit(*node.condition);
  if (condType != ValueType::BOOLEAN && condType != ValueType::UNKNOWN) {
    reportError("Condition of 'if' must be a boolean expression "
                "(use ==, !=, <, >, <=, >=, or !).", 0);
  }

  visit(*node.thenBranch);
  if (node.elseBranch)
    visit(**node.elseBranch);

  return ValueType::UNKNOWN;
}

ValueType SemanticAnalyzer::visitWhile(const WhileNode& node) {
  ValueType condType = visit(*node.condition);
  if (condType != ValueType::BOOLEAN && condType != ValueType::UNKNOWN) {
    reportError("Condition of 'while' must be a boolean expression.", 0);
  }
  visit(*node.body);
  return ValueType::UNKNOWN;
}

ValueType SemanticAnalyzer::visitProgram(const ProgramNode& node) {
  for (auto& stmt : node.statements)
    visit(*stmt);
  return ValueType::UNKNOWN;
}
