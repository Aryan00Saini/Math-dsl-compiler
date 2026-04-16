#include "ast_printer.h"
#include "common.h"
#include <sstream>
#include <string>
#include <variant>

// ── Helpers ───────────────────────────────────────────────────────────────────

static const char* opStr(TokenType op) {
  switch (op) {
    case TokenType::PLUS:          return "+";
    case TokenType::MINUS:         return "-";
    case TokenType::STAR:          return "*";
    case TokenType::SLASH:         return "/";
    case TokenType::CARET:         return "^";
    case TokenType::LESS:          return "<";
    case TokenType::LESS_EQUAL:    return "<=";
    case TokenType::GREATER:       return ">";
    case TokenType::GREATER_EQUAL: return ">=";
    case TokenType::EQUAL_EQUAL:   return "==";
    case TokenType::BANG_EQUAL:    return "!=";
    case TokenType::BANG:          return "!";
    default:                       return "?";
  }
}

// ── Recursive printer ─────────────────────────────────────────────────────────
//
//  prefix      = the indentation string for the current node's children
//  isLast      = true if this node is the last child of its parent
//                (controls whether to use └── or ├──)
//
static void printNode(std::ostringstream& out,
                      const ASTNode& node,
                      const std::string& prefix,
                      bool isLast)
{
  // Branch connector
  out << prefix;
  out << (isLast ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "   // └──
                 : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 "); // ├──

  // Child prefix for this node's own children
  const std::string childPfx = prefix +
      (isLast ? "    " : "\xe2\x94\x82   ");  // │

  std::visit([&](auto& n) {
    using T = std::decay_t<decltype(n)>;

    if constexpr (std::is_same_v<T, NumberNode>) {
      out << "Number(" << n.value << ")\n";
    }
    else if constexpr (std::is_same_v<T, VariableNode>) {
      out << "Variable(" << n.name << ")\n";
    }
    else if constexpr (std::is_same_v<T, BinaryNode>) {
      out << "BinaryOp(" << opStr(n.op) << ")\n";
      printNode(out, *n.left,  childPfx, false);
      printNode(out, *n.right, childPfx, true);
    }
    else if constexpr (std::is_same_v<T, UnaryNode>) {
      out << "UnaryOp(" << opStr(n.op) << ")\n";
      printNode(out, *n.right, childPfx, true);
    }
    else if constexpr (std::is_same_v<T, AssignmentNode>) {
      out << (n.isDecl ? "Let [" : "Assign [") << n.name << "]\n";
      printNode(out, *n.value, childPfx, true);
    }
    else if constexpr (std::is_same_v<T, CallNode>) {
      out << "Call(" << n.callee << ")\n";
      for (size_t i = 0; i < n.args.size(); ++i)
        printNode(out, *n.args[i], childPfx, i + 1 == n.args.size());
    }
    else if constexpr (std::is_same_v<T, BlockNode>) {
      out << "Block\n";
      for (size_t i = 0; i < n.statements.size(); ++i)
        printNode(out, *n.statements[i], childPfx, i + 1 == n.statements.size());
    }
    else if constexpr (std::is_same_v<T, IfNode>) {
      out << "If\n";
      // condition
      out << childPfx << "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 [cond]\n";
      printNode(out, *n.condition,  childPfx + "\xe2\x94\x82   ", true);
      // then
      bool hasElse = n.elseBranch.has_value();
      out << childPfx << (hasElse ? "\xe2\x94\x9c" : "\xe2\x94\x94")
          << "\xe2\x94\x80\xe2\x94\x80 [then]\n";
      printNode(out, *n.thenBranch,
                childPfx + (hasElse ? "\xe2\x94\x82   " : "    "), true);
      if (hasElse) {
        out << childPfx << "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 [else]\n";
        printNode(out, **n.elseBranch, childPfx + "    ", true);
      }
    }
    else if constexpr (std::is_same_v<T, WhileNode>) {
      out << "While\n";
      out << childPfx << "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 [cond]\n";
      printNode(out, *n.condition, childPfx + "\xe2\x94\x82   ", true);
      out << childPfx << "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 [body]\n";
      printNode(out, *n.body, childPfx + "    ", true);
    }
    else if constexpr (std::is_same_v<T, ProgramNode>) {
      // Should not reach here — Program is the root
      out << "Program\n";
    }
  }, node.data);
}

// ── Public API ────────────────────────────────────────────────────────────────

std::string prettyPrintAST(const ASTNode& root) {
  std::ostringstream out;

  if (auto* prog = std::get_if<ProgramNode>(&root.data)) {
    out << "Program\n";
    for (size_t i = 0; i < prog->statements.size(); ++i)
      printNode(out, *prog->statements[i], "", i + 1 == prog->statements.size());
  } else {
    // Single node (unlikely but safe)
    printNode(out, root, "", true);
  }

  return out.str();
}
