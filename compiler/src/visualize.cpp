#include "visualize.h"
#include <cstdio>
#include <iostream>
#include <stdexcept>

static int nodeCounter = 0;

// ── Helper: token type → human-readable operator string ──────────────────────
static const char* opToStr(TokenType op) {
  switch (op) {
    case TokenType::PLUS:            return "+";
    case TokenType::MINUS:           return "-";
    case TokenType::STAR:            return "*";
    case TokenType::SLASH:           return "/";
    case TokenType::CARET:           return "^";
    case TokenType::EQUALS:          return "=";
    case TokenType::EQUAL_EQUAL:     return "==";
    case TokenType::BANG_EQUAL:      return "!=";
    case TokenType::LESS:            return "<";
    case TokenType::LESS_EQUAL:      return "<=";
    case TokenType::GREATER:         return ">";
    case TokenType::GREATER_EQUAL:   return ">=";
    case TokenType::BANG:            return "!";
    default:                         return "?";
  }
}

static int printNode(FILE* fp, const ASTNode& node);

static int printNode(FILE* fp, const ASTNode& node) {
  int id = nodeCounter++;

  std::visit([&](auto&& n) {
    using T = std::decay_t<decltype(n)>;

    // ── Program ──────────────────────────────────────────────────────────────
    if constexpr (std::is_same_v<T, ProgramNode>) {
      fprintf(fp, "  node%d [label=\"Program\", shape=folder, "
                  "style=filled, fillcolor=lightgrey];\n", id);
      for (auto& stmt : n.statements) {
        int child = printNode(fp, *stmt);
        fprintf(fp, "  node%d -> node%d;\n", id, child);
      }
    }
    // ── Number ────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, NumberNode>) {
      fprintf(fp, "  node%d [label=\"%g\", shape=ellipse];\n", id, n.value);
    }
    // ── Variable ──────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, VariableNode>) {
      fprintf(fp, "  node%d [label=\"%s\", shape=box, "
                  "style=filled, fillcolor=lightblue];\n", id, n.name.c_str());
    }
    // ── Binary ────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, BinaryNode>) {
      fprintf(fp, "  node%d [label=\"%s\", shape=circle, "
                  "style=filled, fillcolor=orange];\n", id, opToStr(n.op));
      int l = printNode(fp, *n.left);
      int r = printNode(fp, *n.right);
      fprintf(fp, "  node%d -> node%d;\n", id, l);
      fprintf(fp, "  node%d -> node%d;\n", id, r);
    }
    // ── Unary ─────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, UnaryNode>) {
      fprintf(fp, "  node%d [label=\"unary %s\", shape=circle, "
                  "style=filled, fillcolor=pink];\n", id, opToStr(n.op));
      int r = printNode(fp, *n.right);
      fprintf(fp, "  node%d -> node%d;\n", id, r);
    }
    // ── Assignment ────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, AssignmentNode>) {
      const char* lbl = n.isDecl ? "let =" : "=";
      const char* clr = n.isDecl ? "palegreen" : "lightgreen";
      fprintf(fp, "  node%d [label=\"%s\", shape=circle, "
                  "style=filled, fillcolor=%s];\n", id, lbl, clr);
      // Variable name as leaf
      int varId = nodeCounter++;
      fprintf(fp, "  node%d [label=\"%s\", shape=box, "
                  "style=filled, fillcolor=lightblue];\n", varId, n.name.c_str());
      int valId = printNode(fp, *n.value);
      fprintf(fp, "  node%d -> node%d;\n", id, varId);
      fprintf(fp, "  node%d -> node%d;\n", id, valId);
    }
    // ── Call ──────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, CallNode>) {
      fprintf(fp, "  node%d [label=\"call %s()\", shape=component, "
                  "style=filled, fillcolor=lightyellow];\n", id, n.callee.c_str());
      for (auto& arg : n.args) {
        int argId = printNode(fp, *arg);
        fprintf(fp, "  node%d -> node%d;\n", id, argId);
      }
    }
    // ── Block ─────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, BlockNode>) {
      fprintf(fp, "  node%d [label=\"block\", shape=rectangle, "
                  "style=filled, fillcolor=lightyellow];\n", id);
      for (auto& stmt : n.statements) {
        int child = printNode(fp, *stmt);
        fprintf(fp, "  node%d -> node%d;\n", id, child);
      }
    }
    // ── If ────────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, IfNode>) {
      fprintf(fp, "  node%d [label=\"if\", shape=diamond, "
                  "style=filled, fillcolor=lightsalmon];\n", id);
      int condId = printNode(fp, *n.condition);
      int thenId = printNode(fp, *n.thenBranch);
      fprintf(fp, "  node%d -> node%d [label=\"cond\"];\n", id, condId);
      fprintf(fp, "  node%d -> node%d [label=\"then\"];\n", id, thenId);
      if (n.elseBranch) {
        int elseId = printNode(fp, **n.elseBranch);
        fprintf(fp, "  node%d -> node%d [label=\"else\"];\n", id, elseId);
      }
    }
    // ── While ─────────────────────────────────────────────────────────────────
    else if constexpr (std::is_same_v<T, WhileNode>) {
      fprintf(fp, "  node%d [label=\"while\", shape=diamond, "
                  "style=filled, fillcolor=plum];\n", id);
      int condId = printNode(fp, *n.condition);
      int bodyId = printNode(fp, *n.body);
      fprintf(fp, "  node%d -> node%d [label=\"cond\"];\n", id, condId);
      fprintf(fp, "  node%d -> node%d [label=\"body\"];\n", id, bodyId);
    }
  }, node.data);

  return id;
}

// ── Public API ────────────────────────────────────────────────────────────────

void visualizeAST(const ASTNode& root, const std::string& filename) {
  FILE* fp = fopen(filename.c_str(), "w");
  if (!fp) {
    std::cerr << "Could not open " << filename << " for writing.\n";
    return;
  }

  fprintf(fp, "digraph AST {\n");
  fprintf(fp, "  graph [fontname=\"Helvetica\"];\n");
  fprintf(fp, "  node  [fontname=\"Helvetica\", fontsize=12];\n");
  fprintf(fp, "  edge  [fontname=\"Helvetica\", fontsize=10];\n");
  nodeCounter = 0;
  printNode(fp, root);
  fprintf(fp, "}\n");

  fclose(fp);
}
