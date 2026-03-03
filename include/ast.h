#pragma once
#include "common.h"

// Forward declaration for recursive structures
typedef struct ASTNode ASTNode;

// Data structure for the AST Node
struct ASTNode {
  ASTNodeType type;

  // Depending on the type, only certain parts of this union are active
  union {
    // AST_NUMBER
    struct {
      double value;
    } as_number;

    // AST_VARIABLE
    struct {
      char *name;
    } as_variable;

    // AST_BINARY_OP and AST_ASSIGNMENT
    struct {
      TokenType operator_token;
      ASTNode *left;
      ASTNode *right;
    } as_binary;

    // AST_UNARY_OP
    struct {
      TokenType operator_token;
      ASTNode *right;
    } as_unary;

    // AST_PROGRAM
    struct {
      ASTNode **statements;
      int count;
      int capacity;
    } as_program;
  };
};

// Node allocation functions
ASTNode *ast_new_program();
void ast_program_add_statement(ASTNode *program, ASTNode *statement);
ASTNode *ast_new_number(double value);
ASTNode *ast_new_variable(const char *name, size_t length);
ASTNode *ast_new_binary(TokenType op, ASTNode *left, ASTNode *right);
ASTNode *ast_new_unary(TokenType op, ASTNode *right);
ASTNode *ast_new_assignment(const char *name, size_t length, ASTNode *value);

// Memory tracking/freeing
void free_ast(ASTNode *node);
