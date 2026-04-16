cmake_minimum_required(VERSION 3.16)
project(MathDSL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Build options ─────────────────────────────────────────────────────────────
option(BUILD_CLI   "Build the command-line compiler tool" ON)
option(BUILD_GUI   "Build the ImGui IDE frontend"         ON)
option(BUILD_TESTS "Build tests"                          OFF)

# ── Compiler core (static library) ───────────────────────────────────────────
add_subdirectory(compiler)

# ── CLI frontend ──────────────────────────────────────────────────────────────
if(BUILD_CLI)
    add_subdirectory(cli)
endif()

# ── GUI IDE frontend ──────────────────────────────────────────────────────────
if(BUILD_GUI)
    add_subdirectory(gui)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()