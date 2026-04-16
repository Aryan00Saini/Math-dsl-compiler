# ── SDL2 from MSYS2 ucrt64 ───────────────────────────────────────────────────
# Hardcode paths since we know the MSYS2 install location.
set(MSYS2_UCRT64 "C:/msys64/ucrt64")

add_library(SDL2_iface INTERFACE)
# ImGui SDL2 backend uses #include <SDL.h> (not <SDL2/SDL.h>)
# so we point directly to the SDL2 subdirectory
target_include_directories(SDL2_iface INTERFACE "${MSYS2_UCRT64}/include/SDL2")
target_link_libraries(SDL2_iface INTERFACE
    "${MSYS2_UCRT64}/lib/libSDL2.dll.a"
)

# ── Dear ImGui (vendored source) ─────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL2_iface opengl32)
target_compile_options(imgui PRIVATE -w)   # suppress ImGui warnings

# ── ImGuiColorTextEdit (vendored source) ─────────────────────────────────────
set(IMTE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGuiColorTextEdit)

add_library(imgui_te STATIC
    ${IMTE_DIR}/TextEditor.cpp
)
target_include_directories(imgui_te PUBLIC ${IMTE_DIR})
target_link_libraries(imgui_te PUBLIC imgui)
target_compile_options(imgui_te PRIVATE -w)

# ── Math IDE executable ───────────────────────────────────────────────────────
add_executable(math_ide main_gui.cpp)

target_include_directories(math_ide PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor   # stb_image.h
)

target_link_libraries(math_ide PRIVATE
    math_dsl_compiler
    imgui
    imgui_te
)

target_compile_options(math_ide PRIVATE -Wall -Wextra)

# Copy SDL2.dll next to the .exe so it's found at runtime
add_custom_command(TARGET math_ide POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MSYS2_UCRT64}/bin/SDL2.dll"
        $<TARGET_FILE_DIR:math_ide>
    COMMENT "Copying SDL2.dll to output dir"
)
