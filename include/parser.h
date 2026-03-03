#pragma once
#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

typedef struct {
  Lexer *lexer;
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

// Initializes the parser with an active lexer
void init_parser(Parser *parser, Lexer *lexer);

// Parses the token stream into an AST
// Depending on exact language design, you might return an array of ASTNode*
// For now, we return a single statement ASTNode.
ASTNode *parse(Parser *parser);
