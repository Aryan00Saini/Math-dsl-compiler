#include "interpreter.h"
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// ── Environment helpers ───────────────────────────────────────────────────────

void Interpreter::runtimeError(const std::string& msg) {
  result_->errors.push_back(msg);
  throw std::runtime_error(msg);   // unwind — caught in run()
}

void Interpreter::set(const std::string& name, double value, bool isDecl) {
  if (isDecl) {
    // Declare in innermost scope
    scopes_.back()[name] = value;
    // Track global declaration order
    if (scopes_.size() == 1) {
      if (std::find(globalOrder_.begin(), globalOrder_.end(), name)
          == globalOrder_.end())
        globalOrder_.push_back(name);
    }
  } else {
    // Update — walk scope chain outward
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (it->count(name)) {
        (*it)[name] = value;
        if (scopes_.size() == 1 ||
            std::find(globalOrder_.begin(), globalOrder_.end(), name)
                == globalOrder_.end())
          globalOrder_.push_back(name);
        return;
      }
    }
    // Not found — treat as implicit global
    scopes_.front()[name] = value;
    if (std::find(globalOrder_.begin(), globalOrder_.end(), name)
        == globalOrder_.end())
      globalOrder_.push_back(name);
  }
}

double Interpreter::get(const std::string& name, int line) {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) return found->second;
  }
  runtimeError("[line " + std::to_string(line) + "] Undefined variable '"
               + name + "'");
  return 0.0;
}

// ── Entry point ───────────────────────────────────────────────────────────────

ExecResult Interpreter::run(const ASTNode& root) {
  ExecResult res;
  result_ = &res;
  scopes_.clear();
  globalOrder_.clear();
  scopes_.push_back({});  // global scope

  try {
    eval(root);
  } catch (std::runtime_error&) {
    // Error already recorded in result_->errors
  }

  // Collect global variables in declaration order
  auto& global = scopes_.front();
  for (auto& name : globalOrder_) {
    auto it = global.find(name);
    if (it != global.end())
      res.vars.push_back({ it->first, it->second });
  }

  result_ = nullptr;
  return res;
}

// ── Central dispatch ─────────────────────────────────────────────────────────

double Interpreter::eval(const ASTNode& node) {
  return std::visit([&](auto& n) -> double {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, NumberNode>)     return evalNum(n);
    if constexpr (std::is_same_v<T, VariableNode>)   return evalVar(n);
    if constexpr (std::is_same_v<T, BinaryNode>)     return evalBin(n);
    if constexpr (std::is_same_v<T, UnaryNode>)      return evalUna(n);
    if constexpr (std::is_same_v<T, AssignmentNode>) return evalAsgn(n);
    if constexpr (std::is_same_v<T, CallNode>)       return evalCall(n);
    if constexpr (std::is_same_v<T, BlockNode>)     { execBlock(n); return 0.0; }
    if constexpr (std::is_same_v<T, IfNode>)        { execIf(n);    return 0.0; }
    if constexpr (std::is_same_v<T, WhileNode>)     { execWhile(n); return 0.0; }
    if constexpr (std::is_same_v<T, ProgramNode>)   { execProgram(n); return 0.0; }
    return 0.0;
  }, node.data);
}

// ── Per-node evaluators ───────────────────────────────────────────────────────

double Interpreter::evalNum(const NumberNode& n)  { return n.value; }

double Interpreter::evalVar(const VariableNode& n) {
  return get(n.name, n.line);
}

double Interpreter::evalBin(const BinaryNode& n) {
  double l = eval(*n.left);
  double r = eval(*n.right);
  switch (n.op) {
    case TokenType::PLUS:          return l + r;
    case TokenType::MINUS:         return l - r;
    case TokenType::STAR:          return l * r;
    case TokenType::SLASH:
      if (r == 0.0) runtimeError("Division by zero");
      return l / r;
    case TokenType::CARET:         return std::pow(l, r);
    case TokenType::LESS:          return l < r  ? 1.0 : 0.0;
    case TokenType::LESS_EQUAL:    return l <= r ? 1.0 : 0.0;
    case TokenType::GREATER:       return l > r  ? 1.0 : 0.0;
    case TokenType::GREATER_EQUAL: return l >= r ? 1.0 : 0.0;
    case TokenType::EQUAL_EQUAL:   return l == r ? 1.0 : 0.0;
    case TokenType::BANG_EQUAL:    return l != r ? 1.0 : 0.0;
    default: return 0.0;
  }
}

double Interpreter::evalUna(const UnaryNode& n) {
  double v = eval(*n.right);
  if (n.op == TokenType::MINUS) return -v;
  if (n.op == TokenType::BANG)  return v == 0.0 ? 1.0 : 0.0;
  return 0.0;
}

double Interpreter::evalAsgn(const AssignmentNode& n) {
  double v = eval(*n.value);
  set(n.name, v, n.isDecl);
  return v;
}

double Interpreter::evalCall(const CallNode& n) {
  // Evaluate all arguments
  std::vector<double> args;
  for (auto& a : n.args) args.push_back(eval(*a));

  auto arg = [&](int i) { return args.at(static_cast<size_t>(i)); };
  auto& fn = n.callee;

  if (fn == "sin")   return std::sin(arg(0));
  if (fn == "cos")   return std::cos(arg(0));
  if (fn == "tan")   return std::tan(arg(0));
  if (fn == "asin")  return std::asin(arg(0));
  if (fn == "acos")  return std::acos(arg(0));
  if (fn == "atan")  return std::atan(arg(0));
  if (fn == "atan2") return std::atan2(arg(0), arg(1));
  if (fn == "sqrt") {
    if (arg(0) < 0) runtimeError("sqrt of negative number");
    return std::sqrt(arg(0));
  }
  if (fn == "log") {
    if (arg(0) <= 0) runtimeError("log of non-positive number");
    return std::log(arg(0));
  }
  if (fn == "log2")  return std::log2(arg(0));
  if (fn == "log10") return std::log10(arg(0));
  if (fn == "exp")   return std::exp(arg(0));
  if (fn == "floor") return std::floor(arg(0));
  if (fn == "ceil")  return std::ceil(arg(0));
  if (fn == "abs")   return std::abs(arg(0));
  if (fn == "pow")   return std::pow(arg(0), arg(1));
  if (fn == "max")   return std::max(arg(0), arg(1));
  if (fn == "min")   return std::min(arg(0), arg(1));

  runtimeError("Unknown function '" + fn + "'");
  return 0.0;
}

// ── Statement executors ───────────────────────────────────────────────────────

void Interpreter::execProgram(const ProgramNode& n) {
  for (auto& stmt : n.statements) eval(*stmt);
}

void Interpreter::execBlock(const BlockNode& n) {
  scopes_.push_back({});
  try {
    for (auto& stmt : n.statements) eval(*stmt);
  } catch (...) {
    scopes_.pop_back();
    throw;
  }
  scopes_.pop_back();
}

void Interpreter::execIf(const IfNode& n) {
  double cond = eval(*n.condition);
  if (cond != 0.0) {
    eval(*n.thenBranch);
  } else if (n.elseBranch) {
    eval(**n.elseBranch);
  }
}

void Interpreter::execWhile(const WhileNode& n) {
  int iters = 0;
  while (eval(*n.condition) != 0.0) {
    if (++iters > MAX_LOOP_ITERS) {
      runtimeError("Loop exceeded " + std::to_string(MAX_LOOP_ITERS)
                   + " iterations (infinite loop guard)");
    }
    eval(*n.body);
  }
}
