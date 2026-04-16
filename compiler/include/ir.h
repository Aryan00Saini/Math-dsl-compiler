#pragma once
#include <string>
#include <vector>
#include <variant>

// ── IR Addresses ─────────────────────────────────────────────────────────────
// An "address" in Three-Address Code is one of:
//   TempVar  — compiler-generated temporary (t0, t1, …)
//   NamedVar — user-declared variable
//   Literal  — constant number
//   NoAddr   — sentinel / unused operand slot

struct TempVar  { int id; };
struct NamedVar { std::string name; };
struct Literal  { double value; };
struct NoAddr   {};

using Address = std::variant<TempVar, NamedVar, Literal, NoAddr>;

inline std::string addrToString(const Address& a) {
  return std::visit([](auto&& x) -> std::string {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<T, TempVar>)
      return "t" + std::to_string(x.id);
    else if constexpr (std::is_same_v<T, NamedVar>)
      return x.name;
    else if constexpr (std::is_same_v<T, Literal>)
      return std::to_string(x.value);
    else
      return "_";
  }, a);
}

// ── IR Opcodes ────────────────────────────────────────────────────────────────
enum class IROpcode {
  // Arithmetic
  ADD, SUB, MUL, DIV, POW, NEG,
  // Comparison (result is 0.0 or 1.0)
  LT, GT, LE, GE, EQ, NEQ,
  // Data movement
  COPY,           // dst = src1
  // Functions
  CALL,           // dst = callee(args…)
  // Control flow
  LABEL,          // pseudo-instruction: jump target
  JUMP,           // unconditional goto label
  JUMP_IF_FALSE,  // goto label if src1 == 0.0
  // I/O
  PRINT,          // print src1
  RETURN,
};

inline const char* opcodeToString(IROpcode op) {
  switch (op) {
    case IROpcode::ADD:           return "ADD";
    case IROpcode::SUB:           return "SUB";
    case IROpcode::MUL:           return "MUL";
    case IROpcode::DIV:           return "DIV";
    case IROpcode::POW:           return "POW";
    case IROpcode::NEG:           return "NEG";
    case IROpcode::LT:            return "LT";
    case IROpcode::GT:            return "GT";
    case IROpcode::LE:            return "LE";
    case IROpcode::GE:            return "GE";
    case IROpcode::EQ:            return "EQ";
    case IROpcode::NEQ:           return "NEQ";
    case IROpcode::COPY:          return "COPY";
    case IROpcode::CALL:          return "CALL";
    case IROpcode::LABEL:         return "LABEL";
    case IROpcode::JUMP:          return "JUMP";
    case IROpcode::JUMP_IF_FALSE: return "JIF";
    case IROpcode::PRINT:         return "PRINT";
    case IROpcode::RETURN:        return "RETURN";
    default:                      return "?";
  }
}

// ── IR Instruction ────────────────────────────────────────────────────────────
struct IRInstr {
  IROpcode             op     = IROpcode::COPY;
  Address              dst    = NoAddr{};
  Address              src1   = NoAddr{};
  Address              src2   = NoAddr{};
  std::string          label  = {};   // For LABEL / JUMP / JUMP_IF_FALSE
  std::string          callee = {};   // For CALL
  std::vector<Address> args   = {};   // For CALL arguments

  // Convenience constructor for simple arithmetic/comparison instructions
  IRInstr() = default;
  IRInstr(IROpcode op_, Address dst_, Address src1_, Address src2_,
          std::string label_ = {})
      : op(op_), dst(std::move(dst_)), src1(std::move(src1_)),
        src2(std::move(src2_)), label(std::move(label_)) {}
};

using IRProgram = std::vector<IRInstr>;

// Print one instruction as readable text
std::string instrToString(const IRInstr& instr);
void        printIR(const IRProgram& prog);
