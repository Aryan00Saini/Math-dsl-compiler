#pragma once
#include "ast.h"
#include "ir.h"

// ── IR Generator ──────────────────────────────────────────────────────────────
//
// Walks an ASTNode tree and emits a flat IRProgram (Three-Address Code).
// Control flow is linearised via LABEL / JUMP / JUMP_IF_FALSE instructions.
//
class IRGen {
public:
  IRProgram generate(const ASTNode& root);

private:
  IRProgram code_;
  int       tempCount_  = 0;
  int       labelCount_ = 0;

  // ── Helpers ───────────────────────────────────────────────────────────────
  TempVar    newTemp();
  std::string newLabel();

  void emit(IRInstr instr);

  // ── Node emitters (return the Address holding the result) ─────────────────
  Address emitNode      (const ASTNode& node);
  Address emitNumber    (const NumberNode&);
  Address emitVariable  (const VariableNode&);
  Address emitBinary    (const BinaryNode&);
  Address emitUnary     (const UnaryNode&);
  Address emitAssignment(const AssignmentNode&);
  Address emitCall      (const CallNode&);
  Address emitBlock     (const BlockNode&);
  Address emitIf        (const IfNode&);
  Address emitWhile     (const WhileNode&);
  Address emitProgram   (const ProgramNode&);

  static IROpcode tokenToOpcode(TokenType op);
};
