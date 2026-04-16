#pragma once
#include "common.h"
#include <string>
#include <string_view>

// ── Token ─────────────────────────────────────────────────────────────────────
struct Token {
  TokenType       type;
  std::string     value;  // owned copy (safe for error messages and lifetimes)
  int             line;
  int             column;

  Token(TokenType t, std::string v, int l, int c)
      : type(t), value(std::move(v)), line(l), column(c) {}
};

// ── Lexer ─────────────────────────────────────────────────────────────────────
class Lexer {
public:
  explicit Lexer(std::string source);

  Token nextToken();
  Token peekToken() const;

private:
  std::string source_;  // owns the source buffer
  size_t      start_   = 0;
  size_t      current_ = 0;
  int         line_    = 1;
  int         column_  = 1;

  bool isAtEnd() const;
  char advance();
  char peek()     const;
  char peekNext() const;
  void skipWhitespace();

  Token makeToken(TokenType type) const;
  Token errorToken(const std::string& message) const;

  static bool isAlpha(char c);
  static bool isDigit(char c);

  TokenType identifierType() const;
  Token     identifier();
  Token     number();
};
