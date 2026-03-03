#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>


void init_lexer(Lexer *lexer, const char *source) {
  lexer->source = source;
  lexer->start = source;
  lexer->current = source;
  lexer->line = 1;
  lexer->column = 1;
}

static bool is_at_end(Lexer *lexer) { return *lexer->current == '\0'; }

static char advance(Lexer *lexer) {
  lexer->column++;
  lexer->current++;
  return lexer->current[-1];
}

static char peek(Lexer *lexer) { return *lexer->current; }

static char peek_next(Lexer *lexer) {
  if (is_at_end(lexer))
    return '\0';
  return lexer->current[1];
}

static void skip_whitespace(Lexer *lexer) {
  for (;;) {
    char c = peek(lexer);
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance(lexer);
      break;
    case '\n':
      lexer->line++;
      lexer->column = 1; // Reset column on new line
      advance(lexer);
      break;
    case '/':
      if (peek_next(lexer) == '/') {
        // A comment goes until the end of the line.
        while (peek(lexer) != '\n' && !is_at_end(lexer))
          advance(lexer);
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static Token make_token(Lexer *lexer, TokenType type) {
  Token token;
  token.type = type;
  token.start = lexer->start;
  token.length = (size_t)(lexer->current - lexer->start);
  // The column where the token *started*.
  token.line = lexer->line;
  // We compute the starting column based on current position and token length.
  token.column = lexer->column - token.length;
  return token;
}

static Token error_token(Lexer *lexer, const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = strlen(message);
  token.line = lexer->line;
  token.column = lexer->column - 1;
  return token;
}

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static TokenType check_keyword(Lexer *lexer, int start, int length,
                               const char *rest, TokenType type) {
  if (lexer->current - lexer->start == start + length &&
      memcmp(lexer->start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer *lexer) {
  // Simple Trie/Switch for keywords
  switch (lexer->start[0]) {
  case 'f':
    return check_keyword(lexer, 1, 1, "n", TOKEN_FN);
  case 'l':
    return check_keyword(lexer, 1, 2, "et", TOKEN_LET);
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier(Lexer *lexer) {
  while (is_alpha(peek(lexer)) || is_digit(peek(lexer))) {
    advance(lexer);
  }
  return make_token(lexer, identifier_type(lexer));
}

static Token number(Lexer *lexer) {
  while (is_digit(peek(lexer))) {
    advance(lexer);
  }

  // Look for a fractional part.
  if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
    // Consume the "."
    advance(lexer);

    while (is_digit(peek(lexer))) {
      advance(lexer);
    }
  }

  return make_token(lexer, TOKEN_NUMBER);
}

Token next_token(Lexer *lexer) {
  skip_whitespace(lexer);
  lexer->start = lexer->current;

  if (is_at_end(lexer))
    return make_token(lexer, TOKEN_EOF);

  char c = advance(lexer);

  if (is_alpha(c))
    return identifier(lexer);
  if (is_digit(c))
    return number(lexer);

  switch (c) {
  case '+':
    return make_token(lexer, TOKEN_PLUS);
  case '-':
    return make_token(lexer, TOKEN_MINUS);
  case '*':
    return make_token(lexer, TOKEN_STAR);
  case '/':
    return make_token(lexer, TOKEN_SLASH);
  case '(':
    return make_token(lexer, TOKEN_LEFT_PAREN);
  case ')':
    return make_token(lexer, TOKEN_RIGHT_PAREN);
  case '=':
    return make_token(lexer, TOKEN_EQUALS);
  case ';':
    return make_token(lexer, TOKEN_SEMICOLON);
  case ',':
    return make_token(lexer, TOKEN_COMMA);
  }

  fprintf(stderr, "Lexer Error: Unexpected character '%c' at Line %d, Col %d\n",
          c, lexer->line, lexer->column - 1);
  return error_token(lexer, "Unexpected character.");
}
