#include "lexer.h"
#include <cctype>
#include <iostream>

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

bool Lexer::isAtEnd() const {
  return current_ >= source_.size();
}

char Lexer::advance() {
  ++column_;
  return source_[current_++];
}

char Lexer::peek() const {
  if (isAtEnd()) return '\0';
  return source_[current_];
}

char Lexer::peekNext() const {
  if (current_ + 1 >= source_.size()) return '\0';
  return source_[current_ + 1];
}

void Lexer::skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ': case '\r': case '\t':
        advance();
        break;
      case '\n':
        ++line_;
        column_ = 1;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // Single-line comment — skip to end of line
          while (peek() != '\n' && !isAtEnd())
            advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

Token Lexer::makeToken(TokenType type) const {
  std::string val = source_.substr(start_, current_ - start_);
  int col = column_ - static_cast<int>(val.size());
  return Token(type, std::move(val), line_, col);
}

Token Lexer::errorToken(const std::string& message) const {
  return Token(TokenType::ERROR, message, line_, column_ - 1);
}

bool Lexer::isAlpha(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::isDigit(char c) {
  return std::isdigit(static_cast<unsigned char>(c));
}

TokenType Lexer::identifierType() const {
  std::string word = source_.substr(start_, current_ - start_);
  if (word == "let")   return TokenType::LET;
  if (word == "fn")    return TokenType::FN;
  if (word == "if")    return TokenType::IF;
  if (word == "else")  return TokenType::ELSE;
  if (word == "while") return TokenType::WHILE;
  if (word == "return")return TokenType::RETURN;
  if (word == "true")  return TokenType::TRUE_KW;
  if (word == "false") return TokenType::FALSE_KW;
  return TokenType::IDENTIFIER;
}

Token Lexer::identifier() {
  while (isAlpha(peek()) || isDigit(peek()))
    advance();
  return makeToken(identifierType());
}

Token Lexer::number() {
  while (isDigit(peek()))
    advance();

  // Fractional part
  if (peek() == '.' && isDigit(peekNext())) {
    advance();  // consume '.'
    while (isDigit(peek()))
      advance();
  }

  return makeToken(TokenType::NUMBER);
}

Token Lexer::peekToken() const {
  Lexer copy = *this;
  return copy.nextToken();
}

Token Lexer::nextToken() {
  skipWhitespace();
  start_ = current_;

  if (isAtEnd())
    return makeToken(TokenType::EOF_TOKEN);

  char c = advance();

  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  switch (c) {
    case '+': return makeToken(TokenType::PLUS);
    case '-': return makeToken(TokenType::MINUS);
    case '*': return makeToken(TokenType::STAR);
    case '/': return makeToken(TokenType::SLASH);
    case '^': return makeToken(TokenType::CARET);
    case '(': return makeToken(TokenType::LEFT_PAREN);
    case ')': return makeToken(TokenType::RIGHT_PAREN);
    case '{': return makeToken(TokenType::LEFT_BRACE);
    case '}': return makeToken(TokenType::RIGHT_BRACE);
    case ';': return makeToken(TokenType::SEMICOLON);
    case ',': return makeToken(TokenType::COMMA);
    case '=':
      return makeToken(peek() == '=' ? (advance(), TokenType::EQUAL_EQUAL)
                                     : TokenType::EQUALS);
    case '!':
      return makeToken(peek() == '=' ? (advance(), TokenType::BANG_EQUAL)
                                     : TokenType::BANG);
    case '<':
      return makeToken(peek() == '=' ? (advance(), TokenType::LESS_EQUAL)
                                     : TokenType::LESS);
    case '>':
      return makeToken(peek() == '=' ? (advance(), TokenType::GREATER_EQUAL)
                                     : TokenType::GREATER);
  }

  std::cerr << "Lexer Error: Unexpected character '" << c
            << "' at Line " << line_ << ", Col " << (column_ - 1) << "\n";
  return errorToken(std::string("Unexpected character '") + c + "'.");
}
