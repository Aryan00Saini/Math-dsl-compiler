#include "ir_gen.h"
#include <stdexcept>
#include <iostream>

// ── Helpers ───────────────────────────────────────────────────────────────────

TempVar IRGen::newTemp() {
  return TempVar{tempCount_++};
}

std::string IRGen::newLabel() {
  return "L" + std::to_string(labelCount_++);
}

void IRGen::emit(IRInstr instr) {
  code_.push_back(std::move(instr));
}

IROpcode IRGen::tokenToOpcode(TokenType op) {
  switch (op) {
    case TokenType::PLUS:            return IROpcode::ADD;
    case TokenType::MINUS:           return IROpcode::SUB;
    case TokenType::STAR:            return IROpcode::MUL;
    case TokenType::SLASH:           return IROpcode::DIV;
    case TokenType::CARET:           return IROpcode::POW;
    case TokenType::LESS:            return IROpcode::LT;
    case TokenType::GREATER:         return IROpcode::GT;
    case TokenType::LESS_EQUAL:      return IROpcode::LE;
    case TokenType::GREATER_EQUAL:   return IROpcode::GE;
    case TokenType::EQUAL_EQUAL:     return IROpcode::EQ;
    case TokenType::BANG_EQUAL:      return IROpcode::NEQ;
    default:
      throw std::logic_error("tokenToOpcode: unhandled token");
  }
}

// ── Entry point ───────────────────────────────────────────────────────────────

IRProgram IRGen::generate(const ASTNode& root) {
  code_.clear();
  tempCount_  = 0;
  labelCount_ = 0;
  emitNode(root);
  return std::move(code_);
}

// ── Central dispatch ─────────────────────────────────────────────────────────

Address IRGen::emitNode(const ASTNode& node) {
  return std::visit([&](auto& n) -> Address {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, NumberNode>)     return emitNumber(n);
    if constexpr (std::is_same_v<T, VariableNode>)   return emitVariable(n);
    if constexpr (std::is_same_v<T, BinaryNode>)     return emitBinary(n);
    if constexpr (std::is_same_v<T, UnaryNode>)      return emitUnary(n);
    if constexpr (std::is_same_v<T, AssignmentNode>) return emitAssignment(n);
    if constexpr (std::is_same_v<T, CallNode>)       return emitCall(n);
    if constexpr (std::is_same_v<T, BlockNode>)      return emitBlock(n);
    if constexpr (std::is_same_v<T, IfNode>)         return emitIf(n);
    if constexpr (std::is_same_v<T, WhileNode>)      return emitWhile(n);
    if constexpr (std::is_same_v<T, ProgramNode>)    return emitProgram(n);
    return NoAddr{};
  }, node.data);
}

// ── Per-node emitters ────────────────────────────────────────────────────────

Address IRGen::emitNumber(const NumberNode& node) {
  return Literal{node.value};
}

Address IRGen::emitVariable(const VariableNode& node) {
  return NamedVar{node.name};
}

Address IRGen::emitBinary(const BinaryNode& node) {
  Address l   = emitNode(*node.left);
  Address r   = emitNode(*node.right);
  TempVar dst = newTemp();
  emit({ tokenToOpcode(node.op), dst, l, r });
  return dst;
}

Address IRGen::emitUnary(const UnaryNode& node) {
  Address src = emitNode(*node.right);
  TempVar dst = newTemp();

  if (node.op == TokenType::MINUS) {
    emit({ IROpcode::NEG, dst, src, NoAddr{} });
  } else if (node.op == TokenType::BANG) {
    // !x  →  x == 0.0
    emit({ IROpcode::EQ, dst, src, Literal{0.0} });
  }
  return dst;
}

Address IRGen::emitAssignment(const AssignmentNode& node) {
  Address src = emitNode(*node.value);
  NamedVar dst{node.name};
  emit({ IROpcode::COPY, dst, src, NoAddr{} });
  return dst;
}

Address IRGen::emitCall(const CallNode& node) {
  // Evaluate all arguments first
  std::vector<Address> argAddrs;
  for (auto& arg : node.args)
    argAddrs.push_back(emitNode(*arg));

  TempVar dst = newTemp();
  IRInstr instr;
  instr.op     = IROpcode::CALL;
  instr.dst    = dst;
  instr.callee = node.callee;
  instr.args   = std::move(argAddrs);
  emit(std::move(instr));
  return dst;
}

Address IRGen::emitBlock(const BlockNode& node) {
  for (auto& stmt : node.statements)
    emitNode(*stmt);
  return NoAddr{};
}

// if (cond) thenBranch [else elseBranch]
//
// Generated layout:
//   <cond>
//   JIF   falseLabel
//   <thenBranch>
//   JUMP  endLabel
// falseLabel:
//   <elseBranch>   (if present)
// endLabel:
Address IRGen::emitIf(const IfNode& node) {
  Address cond       = emitNode(*node.condition);
  std::string falseL = newLabel();
  std::string endL   = newLabel();

  emit({ IROpcode::JUMP_IF_FALSE, NoAddr{}, cond, NoAddr{}, falseL });
  emitNode(*node.thenBranch);
  emit({ IROpcode::JUMP, NoAddr{}, NoAddr{}, NoAddr{}, endL });

  IRInstr falseLabel;
  falseLabel.op    = IROpcode::LABEL;
  falseLabel.label = falseL;
  emit(std::move(falseLabel));

  if (node.elseBranch)
    emitNode(**node.elseBranch);

  IRInstr endLabel;
  endLabel.op    = IROpcode::LABEL;
  endLabel.label = endL;
  emit(std::move(endLabel));

  return NoAddr{};
}

// while (cond) body
//
// Generated layout:
// startLabel:
//   <cond>
//   JIF   endLabel
//   <body>
//   JUMP  startLabel
// endLabel:
Address IRGen::emitWhile(const WhileNode& node) {
  std::string startL = newLabel();
  std::string endL   = newLabel();

  IRInstr startLabel;
  startLabel.op    = IROpcode::LABEL;
  startLabel.label = startL;
  emit(std::move(startLabel));

  Address cond = emitNode(*node.condition);
  emit({ IROpcode::JUMP_IF_FALSE, NoAddr{}, cond, NoAddr{}, endL });

  emitNode(*node.body);
  emit({ IROpcode::JUMP, NoAddr{}, NoAddr{}, NoAddr{}, startL });

  IRInstr endLabel;
  endLabel.op    = IROpcode::LABEL;
  endLabel.label = endL;
  emit(std::move(endLabel));

  return NoAddr{};
}

Address IRGen::emitProgram(const ProgramNode& node) {
  for (auto& stmt : node.statements)
    emitNode(*stmt);
  return NoAddr{};
}

// ── IR pretty-printer ────────────────────────────────────────────────────────

std::string instrToString(const IRInstr& i) {
  using namespace std::string_literals;
  switch (i.op) {
    case IROpcode::LABEL:
      return i.label + ":";
    case IROpcode::JUMP:
      return "    JUMP  " + i.label;
    case IROpcode::JUMP_IF_FALSE:
      return "    JIF   " + addrToString(i.src1) + "  →  " + i.label;
    case IROpcode::CALL: {
      std::string s = "    " + addrToString(i.dst) + " = " + i.callee + "(";
      for (size_t k = 0; k < i.args.size(); ++k) {
        if (k) s += ", ";
        s += addrToString(i.args[k]);
      }
      return s + ")";
    }
    case IROpcode::COPY:
      return "    " + addrToString(i.dst) + " = " + addrToString(i.src1);
    case IROpcode::NEG:
      return "    " + addrToString(i.dst) + " = -" + addrToString(i.src1);
    case IROpcode::PRINT:
      return "    PRINT " + addrToString(i.src1);
    case IROpcode::RETURN:
      return "    RETURN " + addrToString(i.src1);
    default:
      return "    " + addrToString(i.dst) + " = " +
             addrToString(i.src1) + " " +
             opcodeToString(i.op) + " " +
             addrToString(i.src2);
  }
}

void printIR(const IRProgram& prog) {
  std::cout << "\n── Three-Address Code IR ──────────────────────────────\n";
  for (auto& instr : prog)
    std::cout << instrToString(instr) << "\n";
  std::cout << "───────────────────────────────────────────────────────\n\n";
}
