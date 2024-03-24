// SPDX-License-Identifier: MIT

#ifndef _FUN_OPENGL_HH_
#define _FUN_OPENGL_HH_

#include "hk.hh"
#include "SDL_timer.h"
#include "SDL_video.h"
using namespace hk;

#include <glad/glad.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifndef DEMO_NAME
#   define DEMO_NAME "(DEMO_NAME not set)"
#endif

struct App {
    Vec2 vp;
    f32 t;
    f32 dt;
};

void demo_init(const App* app);
void demo_frame(const App* app);
void demo_ui(const App* app);

int main(int argc, const char* argv[]) {
    SDL_version sdlv_l; SDL_GetVersion(&sdlv_l);
    SDL_version sdlv_c; SDL_VERSION(&sdlv_c);
    dbglog("SDL v%d.%d.%d (compiled against v%d.%d.%d)",
        sdlv_l.major, sdlv_l.minor, sdlv_l.patch,
        sdlv_c.major, sdlv_c.minor, sdlv_c.patch);
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        dbgerr("Failed to initialize SDL: %s", SDL_GetError());
    }

    // OpenGL 4.3 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // Allow glDebugMessageCallback
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    // Default multisamping
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    const u32 wndflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    SDL_Window* wnd = SDL_CreateWindow("OpenGL: " DEMO_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, wndflags);
    if (!wnd) {
        dbgerr("Failed to create SDL window: %s", SDL_GetError());
    }
    
    SDL_GLContext gl = SDL_GL_CreateContext(wnd);
    if (!gl) {
        dbgerr("Failed to create OpenGL context: %s", SDL_GetError());
    }
    if (SDL_GL_MakeCurrent(wnd, gl) < 0) {
        dbgerr("Failed to activate OpenGL context: %s", SDL_GetError());
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        dbgerr("Failed to load OpenGL functions");
    }

    glDebugMessageCallback([](GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void*) {
        const char* src_s = "<UNKNOWN SOURCE>";
        switch (src) {
        case GL_DEBUG_SOURCE_API:               { src_s = "API"; } break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     { src_s = "WINDOW_SYSTEM"; } break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:   { src_s = "SHADER_COMPILER"; } break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:       { src_s = "THIRD_PARTY"; } break;
        case GL_DEBUG_SOURCE_APPLICATION:       { src_s = "APPLICATION"; } break;
        case GL_DEBUG_SOURCE_OTHER:             { src_s = "OTHER"; } break;
        };

        const char* type_s = "<UNKNOWN TYPE>";
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:               { type_s = "ERROR"; } break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: { type_s = "DEPRECATED_BEHAVIOR"; } break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  { type_s = "UNDEFINED_BEHAVIOR"; } break;
        case GL_DEBUG_TYPE_PORTABILITY:         { type_s = "PORTABILITY"; } break;
        case GL_DEBUG_TYPE_PERFORMANCE:         { type_s = "PERFORMANCE"; } break;
        case GL_DEBUG_TYPE_OTHER:               { type_s = "OTHER"; } break;
        case GL_DEBUG_TYPE_MARKER:              { type_s = "MARKER"; } break;
        };

        switch (severity) {
        case GL_DEBUG_SEVERITY_LOW:
        case GL_DEBUG_SEVERITY_NOTIFICATION: {
            dbglog("GL:%s:%s: %s", src_s, type_s, msg);
        } break;
        default: {
            dbgerr("GL:%s:%s: %s", src_s, type_s, msg);
        } break;
        }
    }, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetStyle().ScrollbarRounding = 0.0f;
    ImGui_ImplSDL2_InitForOpenGL(wnd, gl);
    ImGui_ImplOpenGL3_Init();

    App app = { };

    demo_init(&app);

    SDL_ShowWindow(wnd);
    bool wants_quit = false;
    do {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
        	ImGui_ImplSDL2_ProcessEvent(&evt);
            switch (evt.type) {
            case SDL_QUIT: {
                wants_quit = 1;
            } break;
            case SDL_KEYDOWN: {
                switch (evt.key.keysym.sym) {
                case SDLK_q:
                case SDLK_ESCAPE: {
                	wants_quit = true;
                } break;
                }
            } break;
            }
        }

        {
        	i32 vp_w; i32 vp_h;
        	SDL_GL_GetDrawableSize(wnd, &vp_w, &vp_h);
        	app.vp = Vec2((f32)vp_w, (f32)vp_h);
        }
        static f32 last_t = 0.0f;
        app.t = (f32)SDL_GetTicks64() / 1e3f;
        app.dt = app.t - last_t;
        last_t = app.t;

        demo_frame(&app);

        // Render UI
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(15.0f, 15.0f));
        static bool window_hovered = false;
        if (window_hovered) {
        	ImGui::SetNextWindowSize(ImVec2(500.0f, app.vp.y - 30.0f));
        } else {
        	ImGui::SetNextWindowSize(ImVec2(300.0f, 30.0f));
        }
        if (ImGui::Begin("##debugmenu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            ImGui::Text("Demo name: " DEMO_NAME);
        	

            window_hovered = ImGui::IsWindowHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered();

            if (window_hovered) {
                ImGui::SeparatorText("Demo Properties");
                ImVec2 cur = ImGui::GetCursorPos();
                demo_ui(&app);
                ImVec2 cur2 = ImGui::GetCursorPos();
                if (cur.x == cur2.x && cur.y == cur2.y) {
                    ImGui::TextDisabled("None");
                }

                ImGui::SeparatorText("Application");
                ImGui::Text("Viewport:      %ux%upx", (u32)app.vp.x, (u32)app.vp.y);
                ImGui::Text("Time:          %fs", app.t);
                static f32 last_fps_check = -1.0f;
                static f32 dt = 0.0f;
                static u32 fps = 0;
                if (floor(app.t) != last_fps_check) {
                    last_fps_check = floor(app.t);
                    dt = app.dt;
                    fps = (u32)(1.0f / dt);
                }
                ImGui::Text("Frame time:    %fms (%d FPS)", dt * 1000, fps);
                static bool vsync = SDL_GL_GetSwapInterval() != 0;
                if (ImGui::Checkbox("VSync", &vsync)) {
                    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
                }

                ImGui::SeparatorText("OpenGL");
                ImGui::Text("GL_VERSION:    %s", glGetString(GL_VERSION));
                ImGui::Text("GL_VENDOR:     %s", glGetString(GL_VENDOR));
                ImGui::Text("GL_RENDERER:   %s", glGetString(GL_RENDERER));
                if (ImGui::CollapsingHeader("Extensions")) {
                    static char filter[256] = { };
                    ImGui::InputText("Filter", filter, sizeof(filter));

                    i32 num_extensions; glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
                    for (i32 i = 0; i < num_extensions; ++i) {
                        const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
                        if (!ext) {
                            break;
                        }
                        if (filter[0] != '\0' && !str::icontains(ext, filter)) {
                            continue;
                        }
                        ImGui::Text("%s", ext);
                    }
                }
            }

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(wnd);
    } while (!wants_quit);

    SDL_DestroyWindow(wnd);

    return EXIT_SUCCESS;
}

static inline GLuint compile_gl_program(const char* vs, const char* fs) {
    GLuint prog = glCreateProgram();

    struct {
        GLenum type;
        const char* source;
    } shaders[] = {
        { GL_VERTEX_SHADER,     vs },
        { GL_FRAGMENT_SHADER,   fs },
    };

    for (usize i = 0; i < arrlen(shaders); ++i) {
        GLuint shader = glCreateShader(shaders[i].type);
        glShaderSource(shader, 1, (const GLchar**)&shaders[i].source, NULL);
        glCompileShader(shader);
        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            char log[512];
            glGetShaderInfoLog(shader, sizeof(log), NULL, log);
            dbgerr("Error while compiling shader: %s", log);
        }
        glAttachShader(prog, shader);
    }

    glLinkProgram(prog);
    GLint link_status = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(prog), NULL, log);
        dbgerr("Error while linking program: %s", log);
    }

    dbglog("Compiled shader #%u", prog);

    return prog;
}

#endif // _FUN_OPENGL_HH_