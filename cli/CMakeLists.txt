add_executable(math_dsl main.cpp)

target_link_libraries(math_dsl PRIVATE math_dsl_compiler)

target_compile_options(math_dsl PRIVATE -Wall -Wextra -Wpedantic)
