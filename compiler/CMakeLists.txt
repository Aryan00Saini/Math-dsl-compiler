add_library(math_dsl_compiler STATIC
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/visualize.cpp
    src/semantic_analyzer.cpp
    src/ir_gen.cpp
    src/optimizer.cpp
    src/interpreter.cpp
    src/ast_printer.cpp
)

# 'PUBLIC' makes headers visible to any target that links this library
target_include_directories(math_dsl_compiler
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(math_dsl_compiler
    PRIVATE -Wall -Wextra -Wpedantic
)
