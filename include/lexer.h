#pragma once
#include "common.h"

// Lexer State
typedef struct {
  const char *source;
  const char *start;
  const char *current;
  int line;
  int column;
} Lexer;

// Initialize the lexer with a null-terminated source string
void init_lexer(Lexer *lexer, const char *source);

// Produce the next token from the source code
Token next_token(Lexer *lexer);
