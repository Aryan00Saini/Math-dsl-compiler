#pragma once
#include "common.h"
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Forward declaration
struct ASTNode;
using ASTNodePtr = std::unique_ptr<ASTNode>;

// ── Node types ────────────────────────────────────────────────────────────────

struct NumberNode {
  double value;
};

struct VariableNode {
  std::string name;
  int line   = 0;
  int column = 0;
};

struct BinaryNode {
  TokenType  op;
  ASTNodePtr left;
  ASTNodePtr right;
};

struct UnaryNode {
  TokenType  op;
  ASTNodePtr right;
};

struct AssignmentNode {
  std::string name;     // variable being assigned
  ASTNodePtr  value;
  bool        isDecl  = false;  // true if via 'let'
  int         line    = 0;
};

struct CallNode {
  std::string              callee;   // "sin", "sqrt", user-defined fn, …
  std::vector<ASTNodePtr>  args;
  int                      line = 0;
};

struct BlockNode {
  std::vector<ASTNodePtr> statements;
};

struct IfNode {
  ASTNodePtr                    condition;
  ASTNodePtr                    thenBranch;  // always a BlockNode
  std::optional<ASTNodePtr>     elseBranch;
};

struct WhileNode {
  ASTNodePtr condition;
  ASTNodePtr body;       // always a BlockNode
};

struct ProgramNode {
  std::vector<ASTNodePtr> statements;
};

// ── The unified node ─────────────────────────────────────────────────────────

struct ASTNode {
  ASTNodeType type;
  std::variant<
    NumberNode, VariableNode, BinaryNode, UnaryNode,
    AssignmentNode, CallNode,
    BlockNode, IfNode, WhileNode,
    ProgramNode
  > data;
};

// ── Factory helpers ───────────────────────────────────────────────────────────

ASTNodePtr makeNumber(double value);
ASTNodePtr makeVariable(std::string name, int line = 0, int col = 0);
ASTNodePtr makeBinary(TokenType op, ASTNodePtr left, ASTNodePtr right);
ASTNodePtr makeUnary(TokenType op, ASTNodePtr right);
ASTNodePtr makeAssignment(std::string name, ASTNodePtr value,
                          bool isDecl = false, int line = 0);
ASTNodePtr makeCall(std::string callee, std::vector<ASTNodePtr> args, int line = 0);
ASTNodePtr makeBlock();
ASTNodePtr makeIf(ASTNodePtr cond, ASTNodePtr thenBranch,
                  std::optional<ASTNodePtr> elseBranch = std::nullopt);
ASTNodePtr makeWhile(ASTNodePtr cond, ASTNodePtr body);
ASTNodePtr makeProgram();

void blockAddStatement(ASTNode& block, ASTNodePtr stmt);
void programAddStatement(ASTNode& program, ASTNodePtr stmt);
