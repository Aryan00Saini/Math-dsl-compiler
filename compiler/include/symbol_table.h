#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <stdexcept>

// ── Value types known at compile time ────────────────────────────────────────
enum class ValueType { NUMBER, BOOLEAN, FUNCTION, UNKNOWN };

inline const char* valueTypeName(ValueType vt) {
  switch (vt) {
    case ValueType::NUMBER:   return "number";
    case ValueType::BOOLEAN:  return "boolean";
    case ValueType::FUNCTION: return "function";
    default:                  return "unknown";
  }
}

// ── Symbol entry ──────────────────────────────────────────────────────────────
struct Symbol {
  std::string name;
  ValueType   type     = ValueType::UNKNOWN;
  bool        defined  = false;  // false = declared but never assigned
  int         declLine = 0;
};

// ── Scoped symbol table (stack of hash maps) ──────────────────────────────────
//
// Usage pattern:
//   table.enterScope();          // at every '{'
//   table.declare("x", ...);
//   table.define("x");
//   auto sym = table.lookup("x");
//   table.exitScope();           // at every '}'
//
class SymbolTable {
public:
  SymbolTable() { enterScope(); }  // always start with a global scope

  void enterScope() {
    scopes_.push_back({});
  }

  void exitScope() {
    if (scopes_.size() <= 1)
      throw std::logic_error("Cannot exit global scope");
    scopes_.pop_back();
  }

  // Declare a name in the CURRENT (innermost) scope.
  // Returns false if already declared in this scope.
  bool declare(const std::string& name, ValueType type, int line) {
    auto& current = scopes_.back();
    if (current.count(name)) return false;
    current[name] = Symbol{name, type, false, line};
    return true;
  }

  // Mark a symbol as defined (after assignment).
  // Walks scope chain from innermost outward.
  bool define(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) {
        found->second.defined = true;
        return true;
      }
    }
    return false;
  }

  // Update the inferred ValueType of an existing symbol.
  bool setType(const std::string& name, ValueType type) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) {
        found->second.type = type;
        return true;
      }
    }
    return false;
  }

  // Look up walking the scope chain outward (innermost first).
  std::optional<Symbol> lookup(const std::string& name) const {
    for (auto it = scopes_.crbegin(); it != scopes_.crend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) return found->second;
    }
    return std::nullopt;
  }

  bool isGlobal() const { return scopes_.size() == 1; }
  int  depth()    const { return static_cast<int>(scopes_.size()); }

private:
  // Index 0 = global, back() = current innermost scope
  std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};
