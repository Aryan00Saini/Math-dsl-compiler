#pragma once
#include "ast.h"

// Generate a Graphviz DOT file from the AST
// Output is written to the provided filename.
void visualize_ast(ASTNode *root, const char *filename);
