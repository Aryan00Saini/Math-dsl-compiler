#include "lexer.h"
#include "parser.h"
#include "visualize.h"
#include "semantic_analyzer.h"
#include "ir_gen.h"
#include "optimizer.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::cerr << "Could not open file \"" << path << "\".\n";
    std::exit(74);
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: math_dsl <path-to-source.dsl>\n";
    return 64;
  }

  std::string source = readFile(argv[1]);

  // ── 1. Lex + Parse ───────────────────────────────────────────────────────
  Lexer  lexer(source);
  Parser parser(lexer);

  ASTNodePtr root = parser.parse();

  if (!root || parser.hadError()) {
    std::cerr << "\nParsing failed with " << parser.errors().size() << " error(s).\n";
    return 65;
  }
  std::cout << "✓ Parse OK\n";

  // ── 2. Semantic Analysis ─────────────────────────────────────────────────
  SemanticAnalyzer analyzer;
  bool semOk = analyzer.analyze(*root);

  if (!semOk) {
    std::cerr << "\nSemantic analysis failed with "
              << analyzer.errors().size() << " error(s).\n";
    // Still visualize the AST so the user can inspect the tree
  } else {
    std::cout << "✓ Semantic analysis OK\n";
  }

  // ── 3. AST Visualization ─────────────────────────────────────────────────
  visualizeAST(*root, "ast.dot");
  std::cout << "✓ Generated 'ast.dot'\n";

  int dotRet = std::system("dot -Tpng ast.dot -o ast.png 2>nul");
  if (dotRet == 0)
    std::cout << "✓ Generated 'ast.png'\n";
  else
    std::cout << "  (Graphviz not found — open ast.dot manually)\n";

  if (!semOk) return 66;

  // ── 4. IR Generation ─────────────────────────────────────────────────────
  IRGen irGen;
  IRProgram ir = irGen.generate(*root);
  std::cout << "✓ IR generated (" << ir.size() << " instructions)\n";

  // ── 5. Optimization Passes ───────────────────────────────────────────────
  size_t before = ir.size();
  constantFoldingPass(ir);
  algebraicSimplificationPass(ir);
  deadCodeEliminationPass(ir);
  size_t after = ir.size();

  std::cout << "✓ Optimized: " << before << " → " << after << " instructions";
  if (after < before)
    std::cout << "  (" << (before - after) << " eliminated)";
  std::cout << "\n";

  // ── 6. Print IR ──────────────────────────────────────────────────────────
  printIR(ir);

  return 0;
}
