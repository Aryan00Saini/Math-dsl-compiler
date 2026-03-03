cmake_minimum_required(VERSION 3.15)
project(MathDSL C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

# Executable containing all source files
add_executable(math_dsl
    src/main.c
    src/lexer.c
    src/parser.c
    src/ast.c
    src/visualize.c
)
