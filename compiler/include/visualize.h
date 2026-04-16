#pragma once
#include "ast.h"
#include <string>

// Generate a Graphviz DOT file from the AST (handles all node types).
void visualizeAST(const ASTNode& root, const std::string& filename);
