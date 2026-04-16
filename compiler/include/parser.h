#pragma once
#include "ast.h"
#include "lexer.h"
#include <optional>
#include <vector>

// ── Compile error (carries source location) ──────────────────────────────────
struct ParseError {
  std::string message;
  int         line   = 0;
  int         column = 0;
};

// ── Parser ────────────────────────────────────────────────────────────────────
class Parser {
public:
  explicit Parser(Lexer& lexer);

  // Parse the entire program; returns nullptr on total failure
  ASTNodePtr parse();

  bool                           hadError() const { return !errors_.empty(); }
  const std::vector<ParseError>& errors()   const { return errors_; }

private:
  Lexer&               lexer_;
  Token                current_;
  Token                previous_;
  bool                 panicMode_ = false;
  std::vector<ParseError> errors_;

  // ── Error helpers ────────────────────────────────────────────────────────
  void errorAt(const Token& token, const std::string& message);
  void error(const std::string& message);
  void errorAtCurrent(const std::string& message);

  // ── Token helpers ────────────────────────────────────────────────────────
  void advance();
  void consume(TokenType type, const std::string& message);
  bool check(TokenType type) const;
  bool match(TokenType type);
  void synchronize();

  // ── Recursive descent grammar ────────────────────────────────────────────
  // statement  → ifStmt | whileStmt | letDecl | exprStmt
  ASTNodePtr statement();
  ASTNodePtr ifStatement();
  ASTNodePtr whileStatement();
  ASTNodePtr block();
  ASTNodePtr letDeclaration();
  ASTNodePtr exprStatement();

  // Expressions (lowest → highest precedence)
  ASTNodePtr assignment();   // IDENTIFIER "=" assignment | equality
  ASTNodePtr equality();     // comparison ( ("==" | "!=") comparison )*
  ASTNodePtr comparison();   // expression ( ("<" | ">" | "<=" | ">=") expr )*
  ASTNodePtr expression();   // term ( ("+" | "-") term )*
  ASTNodePtr term();         // power ( ("*" | "/") power )*
  ASTNodePtr power();        // unary ("^" power)?   ← right-recursive
  ASTNodePtr unary();        // ("!" | "-") unary | call
  ASTNodePtr call();         // primary ( "(" args ")" )?
  ASTNodePtr primary();
};
