#include "ast.h"
#include <stdlib.h>
#include <string.h>

static ASTNode *allocate_node(ASTNodeType type) {
  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  if (node) {
    node->type = type;
  }
  return node;
}

ASTNode *ast_new_program() {
  ASTNode *node = allocate_node(AST_PROGRAM);
  if (!node)
    return NULL;
  node->as_program.capacity = 8;
  node->as_program.count = 0;
  node->as_program.statements = (ASTNode **)malloc(sizeof(ASTNode *) * 8);
  return node;
}

void ast_program_add_statement(ASTNode *program, ASTNode *statement) {
  if (program->type != AST_PROGRAM)
    return;

  if (program->as_program.count >= program->as_program.capacity) {
    program->as_program.capacity *= 2;
    program->as_program.statements =
        (ASTNode **)realloc(program->as_program.statements,
                            sizeof(ASTNode *) * program->as_program.capacity);
  }

  program->as_program.statements[program->as_program.count++] = statement;
}

ASTNode *ast_new_number(double value) {
  ASTNode *node = allocate_node(AST_NUMBER);
  if (!node)
    return NULL;
  node->as_number.value = value;
  return node;
}

ASTNode *ast_new_variable(const char *name, size_t length) {
  ASTNode *node = allocate_node(AST_VARIABLE);
  if (!node)
    return NULL;
  // We allocate an extra byte for the null terminator.
  node->as_variable.name = (char *)malloc(length + 1);
  if (node->as_variable.name) {
    strncpy(node->as_variable.name, name, length);
    node->as_variable.name[length] = '\0';
  }
  return node;
}

ASTNode *ast_new_binary(TokenType op, ASTNode *left, ASTNode *right) {
  ASTNode *node = allocate_node(AST_BINARY_OP);
  if (!node)
    return NULL;
  node->as_binary.operator_token = op;
  node->as_binary.left = left;
  node->as_binary.right = right;
  return node;
}

ASTNode *ast_new_unary(TokenType op, ASTNode *right) {
  ASTNode *node = allocate_node(AST_UNARY_OP);
  if (!node)
    return NULL;
  node->as_unary.operator_token = op;
  node->as_unary.right = right;
  return node;
}

ASTNode *ast_new_assignment(const char *name, size_t length, ASTNode *value) {
  ASTNode *node = allocate_node(AST_ASSIGNMENT);
  if (!node)
    return NULL;

  // Left side of assignment is the variable name (for simplicity)
  ASTNode *var_node = ast_new_variable(name, length);

  node->as_binary.operator_token = TOKEN_EQUALS;
  node->as_binary.left = var_node;
  node->as_binary.right = value;

  return node;
}

void free_ast(ASTNode *node) {
  if (!node)
    return;

  switch (node->type) {
  case AST_PROGRAM:
    for (int i = 0; i < node->as_program.count; i++) {
      free_ast(node->as_program.statements[i]);
    }
    free(node->as_program.statements);
    break;
  case AST_NUMBER:
    break; // Nothing extra to free
  case AST_VARIABLE:
    free((void *)node->as_variable.name);
    break;
  case AST_BINARY_OP:
  case AST_ASSIGNMENT: // Assignment structure is identical to binary op
    free_ast(node->as_binary.left);
    free_ast(node->as_binary.right);
    break;
  case AST_UNARY_OP:
    free_ast(node->as_unary.right);
    break;
  }

  free(node);
}
