#pragma once

// ── Token Types ───────────────────────────────────────────────────────────────
enum class TokenType {
  // Single-character tokens
  PLUS, MINUS, STAR, SLASH, CARET,          // + - * / ^
  LEFT_PAREN, RIGHT_PAREN,                  // ( )
  LEFT_BRACE, RIGHT_BRACE,                  // { }
  EQUALS, COMMA, SEMICOLON,                 // = , ;
  BANG,                                     // !

  // Two-character tokens
  BANG_EQUAL,                               // !=
  EQUAL_EQUAL,                              // ==
  LESS, LESS_EQUAL,                         // <  <=
  GREATER, GREATER_EQUAL,                   // >  >=

  // Literals
  IDENTIFIER, NUMBER,

  // Keywords
  LET, FN, IF, ELSE, WHILE, RETURN, TRUE_KW, FALSE_KW,

  // Special
  EOF_TOKEN, ERROR
};

// ── AST Node Types ────────────────────────────────────────────────────────────
enum class ASTNodeType {
  NUMBER, VARIABLE, BINARY_OP, UNARY_OP,
  ASSIGNMENT, CALL,
  IF, WHILE, BLOCK,
  PROGRAM
};
