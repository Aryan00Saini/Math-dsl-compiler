#include "optimizer.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

// ── Constant Folding ─────────────────────────────────────────────────────────
//
// If both src1 and src2 are Literals, compute the result at compile time
// and replace the instruction with a COPY of the constant.
//
void constantFoldingPass(IRProgram& prog) {
  for (auto& instr : prog) {
    auto* l = std::get_if<Literal>(&instr.src1);
    auto* r = std::get_if<Literal>(&instr.src2);
    if (!l || !r) continue;

    double result = 0.0;
    bool   folded = true;

    switch (instr.op) {
      case IROpcode::ADD: result = l->value + r->value; break;
      case IROpcode::SUB: result = l->value - r->value; break;
      case IROpcode::MUL: result = l->value * r->value; break;
      case IROpcode::DIV:
        if (r->value == 0.0) { folded = false; break; }
        result = l->value / r->value;
        break;
      case IROpcode::POW: result = std::pow(l->value, r->value); break;
      case IROpcode::LT:  result = l->value <  r->value ? 1.0 : 0.0; break;
      case IROpcode::GT:  result = l->value >  r->value ? 1.0 : 0.0; break;
      case IROpcode::LE:  result = l->value <= r->value ? 1.0 : 0.0; break;
      case IROpcode::GE:  result = l->value >= r->value ? 1.0 : 0.0; break;
      case IROpcode::EQ:  result = l->value == r->value ? 1.0 : 0.0; break;
      case IROpcode::NEQ: result = l->value != r->value ? 1.0 : 0.0; break;
      default:            folded = false; break;
    }

    if (folded) {
      instr.op   = IROpcode::COPY;
      instr.src1 = Literal{result};
      instr.src2 = NoAddr{};
    }
  }
}

// ── Algebraic Simplification ─────────────────────────────────────────────────
//
// Eliminate trivial arithmetic identities.
//
void algebraicSimplificationPass(IRProgram& prog) {
  for (auto& instr : prog) {
    auto* rLit = std::get_if<Literal>(&instr.src2);
    auto* lLit = std::get_if<Literal>(&instr.src1);

    if (instr.op == IROpcode::MUL) {
      // x * 1  →  x
      if (rLit && rLit->value == 1.0) {
        instr.op = IROpcode::COPY; instr.src2 = NoAddr{}; continue;
      }
      // x * 0  →  0
      if (rLit && rLit->value == 0.0) {
        instr.op = IROpcode::COPY; instr.src1 = Literal{0.0};
        instr.src2 = NoAddr{}; continue;
      }
      // 1 * x  →  x
      if (lLit && lLit->value == 1.0) {
        instr.op = IROpcode::COPY; instr.src1 = instr.src2;
        instr.src2 = NoAddr{}; continue;
      }
      // 0 * x  →  0
      if (lLit && lLit->value == 0.0) {
        instr.op = IROpcode::COPY; instr.src2 = NoAddr{}; continue;
      }
    }

    if (instr.op == IROpcode::ADD) {
      // x + 0  →  x
      if (rLit && rLit->value == 0.0) {
        instr.op = IROpcode::COPY; instr.src2 = NoAddr{}; continue;
      }
      // 0 + x  →  x
      if (lLit && lLit->value == 0.0) {
        instr.op = IROpcode::COPY; instr.src1 = instr.src2;
        instr.src2 = NoAddr{}; continue;
      }
    }

    if (instr.op == IROpcode::SUB) {
      // x - 0  →  x
      if (rLit && rLit->value == 0.0) {
        instr.op = IROpcode::COPY; instr.src2 = NoAddr{}; continue;
      }
    }

    if (instr.op == IROpcode::DIV) {
      // x / 1  →  x
      if (rLit && rLit->value == 1.0) {
        instr.op = IROpcode::COPY; instr.src2 = NoAddr{}; continue;
      }
    }

    if (instr.op == IROpcode::POW) {
      // x ^ 1  →  x
      if (rLit && rLit->value == 1.0) {
        instr.op = IROpcode::COPY; instr.src2 = NoAddr{}; continue;
      }
      // x ^ 0  →  1
      if (rLit && rLit->value == 0.0) {
        instr.op = IROpcode::COPY; instr.src1 = Literal{1.0};
        instr.src2 = NoAddr{}; continue;
      }
    }
  }
}

// ── Dead-Code Elimination ─────────────────────────────────────────────────────
//
// Find TempVar destinations that are never read and remove their instructions.
// Single-pass — does not handle chains (e.g. t0 used only to compute t1,
// and t1 itself is dead). Sufficient for straight-line code.
//
void deadCodeEliminationPass(IRProgram& prog) {
  // Collect all TempVars that appear as a source anywhere
  std::unordered_set<int> usedTemps;
  for (auto& instr : prog) {
    auto collectAddr = [&](const Address& a) {
      if (auto* tv = std::get_if<TempVar>(&a))
        usedTemps.insert(tv->id);
    };
    collectAddr(instr.src1);
    collectAddr(instr.src2);
    for (auto& arg : instr.args)
      collectAddr(arg);
  }

  // Erase instructions whose TempVar destination is never used
  prog.erase(
    std::remove_if(prog.begin(), prog.end(), [&](const IRInstr& instr) {
      if (auto* tv = std::get_if<TempVar>(&instr.dst))
        return !usedTemps.count(tv->id);
      return false;
    }),
    prog.end()
  );
}
