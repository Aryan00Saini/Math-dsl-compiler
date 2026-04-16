#pragma once
#include "ir.h"

// ── Optimization Passes ────────────────────────────────────────────────────
//
// Each pass takes an IRProgram by reference and mutates it in-place.
// Run passes in sequence after IRGen::generate().
//
// Example pipeline:
//   auto ir = gen.generate(*ast);
//   constantFoldingPass(ir);
//   algebraicSimplificationPass(ir);
//   deadCodeEliminationPass(ir);
//

// Fold compile-time-constant binary ops:
//   ADD  3.0  4.0  →  COPY  7.0
//   MUL  2.0  5.0  →  COPY  10.0   etc.
void constantFoldingPass(IRProgram& prog);

// Eliminate trivial identities:
//   x * 1  →  x        x * 0  →  0
//   x + 0  →  x        x - 0  →  x
//   x ^ 1  →  x        x ^ 0  →  1
void algebraicSimplificationPass(IRProgram& prog);

// Remove COPY instructions whose destination is never read.
// (Simple single-pass dead-store elimination)
void deadCodeEliminationPass(IRProgram& prog);
