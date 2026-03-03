#include "visualize.h"
#include <stdio.h>

static int node_counter = 0;

static const char *op_to_str(TokenType op) {
  switch (op) {
  case TOKEN_PLUS:
    return "+";
  case TOKEN_MINUS:
    return "-";
  case TOKEN_STAR:
    return "*";
  case TOKEN_SLASH:
    return "/";
  case TOKEN_EQUALS:
    return "=";
  default:
    return "?";
  }
}

static int print_ast_node(FILE *fp, ASTNode *node) {
  if (!node)
    return -1;

  int id = node_counter++;

  switch (node->type) {
  case AST_PROGRAM: {
    fprintf(fp,
            "  node%d [label=\"Program\", shape=folder, style=filled, "
            "fillcolor=lightgrey];\n",
            id);
    for (int i = 0; i < node->as_program.count; i++) {
      int child_id = print_ast_node(fp, node->as_program.statements[i]);
      fprintf(fp, "  node%d -> node%d;\n", id, child_id);
    }
    break;
  }
  case AST_NUMBER:
    fprintf(fp, "  node%d [label=\"%g\", shape=ellipse];\n", id,
            node->as_number.value);
    break;
  case AST_VARIABLE:
    fprintf(fp,
            "  node%d [label=\"%s\", shape=box, style=filled, "
            "fillcolor=lightblue];\n",
            id, node->as_variable.name);
    break;
  case AST_BINARY_OP: {
    fprintf(fp,
            "  node%d [label=\"%s\", shape=circle, style=filled, "
            "fillcolor=orange];\n",
            id, op_to_str(node->as_binary.operator_token));
    int left_id = print_ast_node(fp, node->as_binary.left);
    int right_id = print_ast_node(fp, node->as_binary.right);
    fprintf(fp, "  node%d -> node%d;\n", id, left_id);
    fprintf(fp, "  node%d -> node%d;\n", id, right_id);
    break;
  }
  case AST_ASSIGNMENT: {
    fprintf(fp,
            "  node%d [label=\"=\", shape=circle, style=filled, "
            "fillcolor=lightgreen];\n",
            id);
    int left_id = print_ast_node(fp, node->as_binary.left);
    int right_id = print_ast_node(fp, node->as_binary.right);
    fprintf(fp, "  node%d -> node%d;\n", id, left_id);
    fprintf(fp, "  node%d -> node%d;\n", id, right_id);
    break;
  }
  case AST_UNARY_OP: {
    fprintf(fp,
            "  node%d [label=\"unary %s\", shape=circle, style=filled, "
            "fillcolor=pink];\n",
            id, op_to_str(node->as_unary.operator_token));
    int right_id = print_ast_node(fp, node->as_unary.right);
    fprintf(fp, "  node%d -> node%d;\n", id, right_id);
    break;
  }
  }

  return id;
}

void visualize_ast(ASTNode *root, const char *filename) {
  if (!root)
    return;

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Could not open %s for writing.\n", filename);
    return;
  }

  fprintf(fp, "digraph AST {\n");
  node_counter = 0; // Reset for new graphs
  print_ast_node(fp, root);
  fprintf(fp, "}\n");

  fclose(fp);
}
