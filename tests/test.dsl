// 1. Variable Scoping and Arithmetic
let x = 10;
let y = (x * 2) + 5;

// 2. Right-Associativity Test (Power Operator)
// This should parse as 2^(3^2) = 2^9 = 512
let p = 2 ^ 3 ^ 2;

// 3. Built-in Functions & Constants
let s = sin(0);
let r = sqrt(16 + 9); // Should fold 16+9 to 25 before sqrt

// 4. Nested Scoping Test
let a = 100;
{
    let a = 50;       // Shadowing global 'a'
    let b = a + 10;   // Should use 50, not 100
}
// 'b' is now out of scope; 'a' is back to 100

// 5. Logical Operators and Control Flow
let count = 0;
let limit = 5;

while (count < limit) {
    if (count == 3) {
        let special = 1;
    } else {
        let special = 0;
    }
    count = count + 1;
}

// 6. Optimization Stress Test
// Constant Folding: 10 + 20 -> 30
// Algebraic Simplification: x * 1 -> x
let opt_test = (10 + 20) * 1; 

// 7. Boolean Unary
let is_valid = !false;