#include "lexer.h"
#include "parser.h"
#include "visualize.h"
#include <stdio.h>
#include <stdlib.h>

// Helper function to read a file into memory
static char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: math_dsl [path]\n");
    return 64;
  }

  const char *filepath = argv[1];
  char *source = read_file(filepath);

  Lexer lexer;
  init_lexer(&lexer, source);

  Parser parser;
  init_parser(&parser, &lexer);

  ASTNode *root = parse(&parser);

  if (root && !parser.had_error) {
    printf("Successfully parsed %s.\n", filepath);
    visualize_ast(root, "ast.dot");
    printf("Generated 'ast.dot' (Graphviz format).\n");

    printf("Running Graphviz to generate 'ast.png'...\n");
    int ret = system("dot -Tpng ast.dot -o ast.png");

    if (ret == 0) {
      printf("Successfully generated 'ast.png'. You can now open it to view "
             "the tree.\n");
    } else {
      printf("Failed to run 'dot'. Ensure Graphviz is installed and in your "
             "system PATH.\n");
    }
  } else {
    printf("Parsing failed.\n");
  }

  free_ast(root);
  free(source);

  return parser.had_error ? 65 : 0;
}
