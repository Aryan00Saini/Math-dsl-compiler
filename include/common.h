#pragma once

#include <stdbool.h>
#include <stddef.h>

// Token types for the Lexer
typedef enum {
  // Single-character tokens
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_EQUALS,
  TOKEN_COMMA,
  TOKEN_SEMICOLON,

  // Literals
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,

  // Keywords (e.g., if we want defining functions or variables)
  TOKEN_LET,
  TOKEN_FN,

  // Special
  TOKEN_EOF,
  TOKEN_ERROR
} TokenType;

// Represents a single token produced by the lexer
typedef struct {
  TokenType type;
  const char *start; // Pointer into the original source string
  size_t length;
  int line;
  int column;
} Token;

// AST Node types
typedef enum {
  AST_NUMBER,
  AST_VARIABLE,
  AST_BINARY_OP,
  AST_UNARY_OP,
  AST_ASSIGNMENT,
  AST_PROGRAM
} ASTNodeType;
