// Math DSL IDE  --  Dear ImGui + SDL2 + OpenGL3
// Layout: [Code Editor] | [Results] / [AST Image]
// All UI strings are ASCII-only (ImGui default font has no Unicode glyphs).

#define SDL_MAIN_HANDLED

// stb_image -- single-header PNG loader
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "TextEditor.h"

// Compiler pipeline
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "ir_gen.h"
#include "optimizer.h"
#include "interpreter.h"
#include "visualize.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  OpenGL texture helpers
// ─────────────────────────────────────────────────────────────────────────────
static GLuint g_astTex  = 0;
static int    g_astTexW = 0;
static int    g_astTexH = 0;

static void reloadAstTexture(const char* path) {
    // Delete old texture
    if (g_astTex) { glDeleteTextures(1, &g_astTex); g_astTex = 0; }

    int w, h, ch;
    unsigned char* px = stbi_load(path, &w, &h, &ch, 4);
    if (!px) return;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);

    g_astTex  = tex;
    g_astTexW = w;
    g_astTexH = h;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Math DSL language definition for TextEditor
// ─────────────────────────────────────────────────────────────────────────────
static TextEditor::LanguageDefinition MathDSLLang() {
    TextEditor::LanguageDefinition lang;
    lang.mName = "MathDSL";

    for (auto kw : { "let","fn","if","else","while","return","true","false" })
        lang.mKeywords.insert(kw);

    for (auto fn : { "sin","cos","tan","asin","acos","atan","sqrt",
                     "pow","log","log2","log10","exp","floor","ceil",
                     "abs","max","min" }) {
        TextEditor::Identifier id;
        id.mDeclaration = "Built-in math function";
        lang.mIdentifiers.insert({ fn, id });
    }

    // Token patterns -- ASCII only; no Unicode regex
    lang.mTokenRegexStrings = {
        { R"(//[^\n]*)",                   TextEditor::PaletteIndex::Comment     },
        { R"([0-9]+(\.[0-9]+)?)",          TextEditor::PaletteIndex::Number      },
        { R"([a-zA-Z_][a-zA-Z0-9_]*)",    TextEditor::PaletteIndex::Identifier  },
        { R"([+\-*/^=<>!,;{}()]+)",        TextEditor::PaletteIndex::Punctuation },
    };

    lang.mSingleLineComment = "//";
    lang.mCommentStart      = "/*";
    lang.mCommentEnd        = "*/";
    lang.mAutoIndentation   = true;
    return lang;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Format a double -- trim trailing zeros
// ─────────────────────────────────────────────────────────────────────────────
static std::string fmtNum(double v) {
    if (v == static_cast<long long>(v) && v > -1e12 && v < 1e12) {
        return std::to_string(static_cast<long long>(v));
    }
    std::ostringstream ss;
    ss << std::setprecision(8) << v;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Compile + interpret pipeline
// ─────────────────────────────────────────────────────────────────────────────
struct IDEResult {
    bool        ok        = false;
    long long   ms        = 0;
    std::string resultsText;
    TextEditor::ErrorMarkers markers;
    bool        astPng    = false;   // true if ast_ide.png was generated
};

static IDEResult runPipeline(const std::string& source, TextEditor& editor) {
    IDEResult res;
    auto t0 = std::chrono::steady_clock::now();

    try {
        // 1. Parse
        Lexer  lex(source);
        Parser par(lex);
        auto   ast = par.parse();

        if (par.hadError()) {
            std::ostringstream o;
            o << "Parse failed\n";
            o << std::string(44, '-') << "\n";
            for (auto& e : par.errors()) {
                o << "  line " << e.line << ": " << e.message << "\n";
                if (e.line > 0) res.markers[e.line] = e.message;
            }
            res.resultsText = o.str();
            editor.SetErrorMarkers(res.markers);
            return res;
        }

        if (!ast) { res.resultsText = "Empty program.\n"; return res; }

        // 2. Semantic
        SemanticAnalyzer sem;
        sem.analyze(*ast);
        bool semOK = !sem.hadError();

        for (auto& e : sem.errors())
            if (e.line > 0) res.markers[e.line] = e.message;

        // 3. AST image (dot -> png)
        visualizeAST(*ast, "ast_ide.dot");
        int dotRet = std::system("dot -Tpng ast_ide.dot -o ast_ide.png 2>nul");
        if (dotRet == 0) {
            reloadAstTexture("ast_ide.png");
            res.astPng = (g_astTex != 0);
        }

        // 4. Interpreter -- produce variable watch table
        {
            Interpreter  interp;
            ExecResult   exec = interp.run(*ast);
            bool execOK = !exec.hadError();

            std::ostringstream o;

            // Semantic errors banner
            if (!semOK) {
                o << "Semantic errors\n";
                o << std::string(44, '-') << "\n";
                for (auto& e : sem.errors())
                    o << "  line " << e.line << ": " << e.message << "\n";
                o << "\n";
            }

            // Runtime errors
            if (!exec.errors.empty()) {
                o << "Runtime errors\n";
                o << std::string(44, '-') << "\n";
                for (auto& e : exec.errors) o << "  " << e << "\n";
                o << "\n";
            }

            // Variable table
            if (!exec.vars.empty()) {
                // Find column width
                size_t col = 12;
                for (auto& v : exec.vars) col = std::max(col, v.name.size() + 2);

                std::string sep(col + 18, '-');
                o << "Variables after execution\n";
                o << sep << "\n";

                // Header
                std::string hdr = "Name";
                hdr += std::string(col - 4, ' ');
                hdr += "Value\n";
                o << hdr;
                o << sep << "\n";

                for (auto& v : exec.vars) {
                    std::string val = fmtNum(v.value);
                    std::string pad(col - v.name.size(), ' ');
                    o << v.name << pad << val << "\n";
                }

                o << sep << "\n";
                o << exec.vars.size() << " variable(s)\n";
            } else {
                o << "No global variables.\n";
            }

            res.resultsText = o.str();
            res.ok = semOK && execOK;
        }

    } catch (std::exception& ex) {
        res.resultsText = std::string("Internal error: ") + ex.what() + "\n";
    }

    auto t1 = std::chrono::steady_clock::now();
    res.ms  = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    editor.SetErrorMarkers(res.markers);
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Dark theme  (all plain ImVec4 -- no Unicode)
// ─────────────────────────────────────────────────────────────────────────────
static void applyStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4*     c = s.Colors;

    s.WindowRounding   = 6.f;  s.ChildRounding    = 5.f;
    s.FrameRounding    = 4.f;  s.GrabRounding     = 4.f;
    s.PopupRounding    = 5.f;  s.TabRounding      = 4.f;
    s.ScrollbarRounding= 5.f;  s.ScrollbarSize    = 12.f;
    s.FramePadding     = {6,4};s.ItemSpacing      = {8,5};
    s.WindowPadding    = {10,8};

    c[ImGuiCol_WindowBg]         = {0.07f,0.08f,0.10f,1.f};
    c[ImGuiCol_ChildBg]          = {0.09f,0.10f,0.13f,1.f};
    c[ImGuiCol_PopupBg]          = {0.10f,0.11f,0.14f,0.98f};
    c[ImGuiCol_Border]           = {0.18f,0.21f,0.27f,0.9f};
    c[ImGuiCol_FrameBg]          = {0.11f,0.12f,0.16f,1.f};
    c[ImGuiCol_TitleBg]          = {0.07f,0.08f,0.11f,1.f};
    c[ImGuiCol_TitleBgActive]    = {0.09f,0.11f,0.15f,1.f};
    c[ImGuiCol_MenuBarBg]        = {0.07f,0.08f,0.11f,1.f};
    c[ImGuiCol_ScrollbarBg]      = {0.06f,0.07f,0.09f,0.5f};
    c[ImGuiCol_ScrollbarGrab]    = {0.22f,0.26f,0.36f,1.f};
    c[ImGuiCol_ScrollbarGrabHovered]={0.28f,0.33f,0.46f,1.f};
    c[ImGuiCol_Button]           = {0.20f,0.38f,0.68f,1.f};
    c[ImGuiCol_ButtonHovered]    = {0.28f,0.48f,0.80f,1.f};
    c[ImGuiCol_ButtonActive]     = {0.16f,0.30f,0.55f,1.f};
    c[ImGuiCol_Tab]              = {0.11f,0.13f,0.17f,1.f};
    c[ImGuiCol_TabHovered]       = {0.20f,0.38f,0.68f,0.8f};
    c[ImGuiCol_TabActive]        = {0.17f,0.33f,0.58f,1.f};
    c[ImGuiCol_Separator]        = {0.18f,0.21f,0.28f,1.f};
    c[ImGuiCol_Text]             = {0.87f,0.90f,0.94f,1.f};
    c[ImGuiCol_TextDisabled]     = {0.42f,0.46f,0.54f,1.f};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Starter code  --  all plain ASCII
// ─────────────────────────────────────────────────────────────────────────────
static const char* kStarter =
"// Math DSL IDE\n"
"// Press F5 or click [ Run ] to compile and execute.\n"
"\n"
"// Variables and arithmetic\n"
"let x = 10;\n"
"let y = (x * 2) + 5;\n"
"\n"
"// Right-associative power: 2^(3^2) = 512\n"
"let p = 2 ^ 3 ^ 2;\n"
"\n"
"// Built-in math functions\n"
"let s = sin(0);\n"
"let r = sqrt(16 + 9);\n"
"\n"
"// Control flow\n"
"let count = 0;\n"
"while (count < 5) {\n"
"    if (count == 3) {\n"
"        let hit = 1;\n"
"    } else {\n"
"        let hit = 0;\n"
"    }\n"
"    count = count + 1;\n"
"}\n"
"\n"
"// Constant folding: (10 + 20) * 1 -> 30\n"
"let opt = (10 + 20) * 1;\n";

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int, char**) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* win = SDL_CreateWindow(
        "Math DSL IDE",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1400, 840,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return 1; }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
    SDL_GL_MakeCurrent(win, gl_ctx);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui_ImplSDL2_InitForOpenGL(win, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 130");
    applyStyle();

    // ── Editor ───────────────────────────────────────────────────────────────
    TextEditor editor;
    editor.SetLanguageDefinition(MathDSLLang());
    editor.SetShowWhitespaces(false);
    editor.SetText(kStarter);

    // ── State ─────────────────────────────────────────────────────────────────
    IDEResult   result;
    result.resultsText = "Press F5 or click [ Run ] to execute.\n";

    bool        lastOK     = true;
    std::string statusText = "Ready";

    const ImVec4 kGreen = {0.28f, 0.85f, 0.45f, 1.f};
    const ImVec4 kRed   = {0.90f, 0.32f, 0.32f, 1.f};
    const ImVec4 kBlue  = {0.45f, 0.70f, 1.00f, 1.f};
    const ImVec4 kDim   = {0.42f, 0.46f, 0.55f, 1.f};
    const ImVec4 kBg2   = {0.07f, 0.08f, 0.10f, 1.f};

    auto doRun = [&]() {
        result    = runPipeline(editor.GetText(), editor);
        lastOK    = result.ok;
        statusText = lastOK
            ? "[OK]  " + std::to_string(result.ms) + " ms"
            : "[!!]  " + std::to_string(result.ms) + " ms  -- see errors";
    };

    // ── Main loop ─────────────────────────────────────────────────────────────
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_F5) doRun();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        int W, H;  SDL_GetWindowSize(win, &W, &H);
        const float fw = float(W), fh = float(H);
        const float pad = 6.f, toolH = 40.f;

        // Full-screen host
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({fw, fh});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::Begin("##host", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar  | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        // ── Menu bar ─────────────────────────────────────────────────────────
        if (ImGui::BeginMenuBar()) {
            ImGui::TextColored(kBlue, " Math DSL IDE");
            ImGui::Separator();
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    editor.SetText("");
                    TextEditor::ErrorMarkers em; editor.SetErrorMarkers(em);
                    result = IDEResult{};
                    result.resultsText = "New file.\n";
                    if (g_astTex) { glDeleteTextures(1, &g_astTex); g_astTex = 0; }
                    statusText = "Ready"; lastOK = true;
                }
                if (ImGui::MenuItem("Quit")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Run")) {
                if (ImGui::MenuItem("Run  (F5)")) doRun();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                bool ws = editor.IsShowingWhitespaces();
                if (ImGui::MenuItem("Show whitespace", nullptr, ws))
                    editor.SetShowWhitespaces(!ws);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        const float bodyTop = ImGui::GetCursorPosY() + 4.f;
        const float panelH  = fh - bodyTop - toolH - pad;
        const float edW     = fw * 0.56f;
        const float outW    = fw - edW - ImGui::GetStyle().ItemSpacing.x - pad * 2.f;

        ImGui::SetCursorPos({pad, bodyTop});

        // ── Left: Code editor ─────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f,0.10f,0.13f,1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
        ImGui::BeginChild("##ed", {edW, panelH}, true);

        ImGui::TextColored(kBlue, " Code Editor");
        auto cur = editor.GetCursorPosition();
        ImGui::SameLine(edW - 130.f);
        ImGui::TextColored(kDim, "Ln %-4d  Col %d", cur.mLine+1, cur.mColumn+1);
        ImGui::Separator();
        editor.Render("##code", {-1.f, ImGui::GetContentRegionAvail().y});

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::SameLine(0, pad);

        // ── Right: results (top) + AST image (bottom) ─────────────────────────
        ImVec4 rightBorder = lastOK
            ? ImVec4(0.14f,0.44f,0.20f,0.9f)
            : ImVec4(0.55f,0.14f,0.14f,0.9f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, kBg2);
        ImGui::PushStyleColor(ImGuiCol_Border, rightBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
        ImGui::BeginChild("##right", {outW, panelH}, true);

        // How to split the right column vertically:
        // - If we have an AST image: 42% results / 58% image
        // - Otherwise: 100% results
        bool hasImg = (g_astTex != 0);
        float resH  = hasImg ? panelH * 0.42f : panelH - 10.f;
        float imgH  = hasImg ? panelH - resH - ImGui::GetStyle().ItemSpacing.y - 30.f : 0.f;

        // Results section
        ImGui::TextColored(kBlue, " Results");
        ImGui::SameLine(outW - 80.f);
        if (ImGui::SmallButton("Copy")) ImGui::SetClipboardText(result.resultsText.c_str());
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f,0.08f,0.10f,1.f));
        ImGui::BeginChild("##res_scroll", {-1, resH - 40.f}, false,
            ImGuiWindowFlags_HorizontalScrollbar);
        ImVec4 resTextColor = lastOK
            ? ImVec4(0.75f,0.95f,0.75f,1.f)
            : ImVec4(0.96f,0.72f,0.72f,1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, resTextColor);
        ImGui::TextUnformatted(result.resultsText.c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // AST image section
        if (hasImg) {
            ImGui::Separator();
            ImGui::TextColored(kBlue, " AST Diagram");
            ImGui::SameLine(outW - 120.f);
            ImGui::TextColored(kDim, "%d x %d px", g_astTexW, g_astTexH);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f,0.09f,0.12f,1.f));
            ImGui::BeginChild("##ast_img", {-1, imgH}, false,
                ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

            // Scale image to fit width; preserve aspect ratio
            float avW   = ImGui::GetContentRegionAvail().x;
            float avH   = ImGui::GetContentRegionAvail().y;
            float scale = std::min(avW  / float(g_astTexW),
                                   avH  / float(g_astTexH));
            if (scale > 1.f) scale = 1.f;  // never upscale

            float dw = float(g_astTexW) * scale;
            float dh = float(g_astTexH) * scale;

            // Center horizontally if narrower than panel
            if (dw < avW) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avW - dw) * 0.5f);

            ImGui::Image(
                static_cast<ImTextureID>(static_cast<uintptr_t>(g_astTex)),
                {dw, dh});

            ImGui::EndChild();
            ImGui::PopStyleColor();
        } else if (result.ok || !result.resultsText.empty()) {
            // Graphviz not installed -- show hint
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextColored(kDim, " AST diagram: install Graphviz to see the image.");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // ── Bottom toolbar ────────────────────────────────────────────────────
        float toolY = fh - toolH;
        ImGui::SetCursorPos({pad, toolY});
        ImGui::Separator();
        ImGui::SetCursorPos({pad, toolY + 7.f});

        // Run
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.12f,0.52f,0.24f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.18f,0.65f,0.32f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.09f,0.40f,0.18f,1.f});
        if (ImGui::Button("  Run  (F5)  ", {130, 26})) doRun();
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, 8);

        // Clear
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.26f,0.12f,0.12f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.40f,0.18f,0.18f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.20f,0.10f,0.10f,1.f});
        if (ImGui::Button("  Clear  ", {80, 26})) {
            TextEditor::ErrorMarkers em; editor.SetErrorMarkers(em);
            result = IDEResult{}; result.resultsText = "";
            if (g_astTex) { glDeleteTextures(1, &g_astTex); g_astTex = 0; }
            statusText = "Ready"; lastOK = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, 16);
        ImGui::SetCursorPosY(toolY + 11.f);
        ImGui::TextColored(lastOK ? kGreen : kRed, "%s", statusText.c_str());
        ImGui::SameLine(fw - 70.f);
        ImGui::SetCursorPosY(toolY + 11.f);
        ImGui::TextColored(kDim, "%.0f fps", io.Framerate);

        ImGui::End();

        // Render
        ImGui::Render();
        glViewport(0, 0, W, H);
        glClearColor(0.07f, 0.08f, 0.10f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    if (g_astTex) glDeleteTextures(1, &g_astTex);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
