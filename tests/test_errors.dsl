// Semantic Error Tests — should all produce clear diagnostics
let x = 10;

// Error 1: use before declare
let y = z + 1;

// Error 2: boolean in arithmetic
let bad = (x > 5) + 3;
