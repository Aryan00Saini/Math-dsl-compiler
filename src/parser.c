#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for recursive descent
static ASTNode *expression(Parser *parser);
static ASTNode *statement(Parser *parser);
static ASTNode *assignment(Parser *parser);
static ASTNode *term(Parser *parser);
static ASTNode *factor(Parser *parser);
static ASTNode *unary(Parser *parser);
static ASTNode *primary(Parser *parser);

static void error_at(Parser *parser, Token *token, const char *message) {
  if (parser->panic_mode)
    return;
  parser->panic_mode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser->had_error = true;
}

static void error(Parser *parser, const char *message) {
  error_at(parser, &parser->previous, message);
}

static void error_at_current(Parser *parser, const char *message) {
  error_at(parser, &parser->current, message);
}

static void advance(Parser *parser) {
  parser->previous = parser->current;

  for (;;) {
    parser->current = next_token(parser->lexer);
    if (parser->current.type != TOKEN_ERROR)
      break;

    error_at_current(parser, parser->current.start);
  }
}

static void consume(Parser *parser, TokenType type, const char *message) {
  if (parser->current.type == type) {
    advance(parser);
    return;
  }
  error_at_current(parser, message);
}

static bool check(Parser *parser, TokenType type) {
  return parser->current.type == type;
}

static bool match(Parser *parser, TokenType type) {
  if (!check(parser, type))
    return false;
  advance(parser);
  return true;
}

static void synchronize(Parser *parser) {
  parser->panic_mode = false;

  while (parser->current.type != TOKEN_EOF) {
    if (parser->previous.type == TOKEN_SEMICOLON)
      return;

    // We sync if we encouter structural keywords.
    switch (parser->current.type) {
    case TOKEN_LET:
    case TOKEN_FN:
      return;
    default:; // Do nothing
    }

    advance(parser);
  }
}

void init_parser(Parser *parser, Lexer *lexer) {
  parser->lexer = lexer;
  parser->had_error = false;
  parser->panic_mode = false;
  // Prime the parser
  advance(parser);
}

// parsing logic:
// statement -> "let" IDENTIFIER "=" expression ";" | expression ";"
static ASTNode *statement(Parser *parser) {
  if (match(parser, TOKEN_LET)) {
    return assignment(parser);
  }

  ASTNode *expr = expression(parser);
  consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
  return expr;
}

static ASTNode *assignment(Parser *parser) {
  consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
  Token name = parser->previous;

  consume(parser, TOKEN_EQUALS, "Expect '=' after variable name.");

  ASTNode *value = expression(parser);
  consume(parser, TOKEN_SEMICOLON, "Expect ';' after assignment.");

  return ast_new_assignment(name.start, name.length, value);
}

// expression -> term ( ( "+" | "-" ) term )*
static ASTNode *expression(Parser *parser) {
  ASTNode *node = term(parser);

  while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
    TokenType op = parser->previous.type;
    ASTNode *right = term(parser);
    node = ast_new_binary(op, node, right);
  }

  return node;
}

// term -> unary ( ( "*" | "/" ) unary )*
static ASTNode *term(Parser *parser) {
  ASTNode *node = unary(parser);

  while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH)) {
    TokenType op = parser->previous.type;
    ASTNode *right = unary(parser);
    node = ast_new_binary(op, node, right);
  }

  return node;
}

// unary -> "-" unary | primary
static ASTNode *unary(Parser *parser) {
  if (match(parser, TOKEN_MINUS)) {
    TokenType op = parser->previous.type;
    ASTNode *right = unary(parser);
    return ast_new_unary(op, right);
  }

  return primary(parser);
}

// primary -> NUMBER | IDENTIFIER | "(" expression ")"
static ASTNode *primary(Parser *parser) {
  if (match(parser, TOKEN_NUMBER)) {
    double value = strtod(parser->previous.start, NULL);
    return ast_new_number(value);
  }

  if (match(parser, TOKEN_IDENTIFIER)) {
    return ast_new_variable(parser->previous.start, parser->previous.length);
  }

  if (match(parser, TOKEN_LEFT_PAREN)) {
    ASTNode *node = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    return node;
  }

  error(parser, "Expect expression.");
  return NULL;
}

ASTNode *parse(Parser *parser) {
  ASTNode *program = ast_new_program();

  while (!check(parser, TOKEN_EOF)) {
    ASTNode *stmt = statement(parser);
    if (stmt) {
      ast_program_add_statement(program, stmt);
    }

    if (parser->had_error) {
      synchronize(parser);
      parser->had_error = false; // Reset error to try parsing next statement
    }
  }

  if (program->as_program.count == 0) {
    free_ast(program);
    return NULL;
  }

  return program;
}
