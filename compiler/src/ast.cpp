#include "ast.h"
#include <stdexcept>

// ── Number ────────────────────────────────────────────────────────────────────
ASTNodePtr makeNumber(double value) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::NUMBER;
  n->data = NumberNode{value};
  return n;
}

// ── Variable ──────────────────────────────────────────────────────────────────
ASTNodePtr makeVariable(std::string name, int line, int col) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::VARIABLE;
  n->data = VariableNode{std::move(name), line, col};
  return n;
}

// ── Binary ────────────────────────────────────────────────────────────────────
ASTNodePtr makeBinary(TokenType op, ASTNodePtr left, ASTNodePtr right) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::BINARY_OP;
  n->data = BinaryNode{op, std::move(left), std::move(right)};
  return n;
}

// ── Unary ─────────────────────────────────────────────────────────────────────
ASTNodePtr makeUnary(TokenType op, ASTNodePtr right) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::UNARY_OP;
  n->data = UnaryNode{op, std::move(right)};
  return n;
}

// ── Assignment ────────────────────────────────────────────────────────────────
ASTNodePtr makeAssignment(std::string name, ASTNodePtr value,
                          bool isDecl, int line) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::ASSIGNMENT;
  n->data = AssignmentNode{std::move(name), std::move(value), isDecl, line};
  return n;
}

// ── Call ──────────────────────────────────────────────────────────────────────
ASTNodePtr makeCall(std::string callee, std::vector<ASTNodePtr> args, int line) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::CALL;
  n->data = CallNode{std::move(callee), std::move(args), line};
  return n;
}

// ── Block ─────────────────────────────────────────────────────────────────────
ASTNodePtr makeBlock() {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::BLOCK;
  n->data = BlockNode{};
  return n;
}

void blockAddStatement(ASTNode& block, ASTNodePtr stmt) {
  std::get<BlockNode>(block.data).statements.push_back(std::move(stmt));
}

// ── If ────────────────────────────────────────────────────────────────────────
ASTNodePtr makeIf(ASTNodePtr cond, ASTNodePtr thenBranch,
                  std::optional<ASTNodePtr> elseBranch) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::IF;
  n->data = IfNode{std::move(cond), std::move(thenBranch), std::move(elseBranch)};
  return n;
}

// ── While ─────────────────────────────────────────────────────────────────────
ASTNodePtr makeWhile(ASTNodePtr cond, ASTNodePtr body) {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::WHILE;
  n->data = WhileNode{std::move(cond), std::move(body)};
  return n;
}

// ── Program ───────────────────────────────────────────────────────────────────
ASTNodePtr makeProgram() {
  auto n = std::make_unique<ASTNode>();
  n->type = ASTNodeType::PROGRAM;
  n->data = ProgramNode{};
  return n;
}

void programAddStatement(ASTNode& program, ASTNodePtr stmt) {
  std::get<ProgramNode>(program.data).statements.push_back(std::move(stmt));
}
