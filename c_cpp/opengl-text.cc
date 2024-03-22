// SPDX-License-Identifier: MIT

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "opengl.hh"

#include "hk.hh"

#include "stb_truetype.h"

using namespace hk;

struct Vertex {
    Vec2 p;
    Vec2 t;
};

constexpr usize BATCH_SIZE = 256;

static struct {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    usize  head;
    Vertex batch[BATCH_SIZE * 4];
} r = { };

constexpr u8 ASCII_START = 32;
constexpr u8 ASCII_END   = 126;
constexpr usize ASCII_SIZE = ASCII_END - ASCII_START + 1;

static stbtt_bakedchar glyphs[ASCII_SIZE];

static void DrawText(Vec2 pos, const char* text, bool flush = false) {
    bool flush_now = flush;
    const char* leftover = nullptr; // text left over after flush
    for (usize i = 0; text && text[i]; ++i) {
        if ((u8)text[i] < ASCII_START || (u8)text[i] > ASCII_END) {
            continue;
        }

        stbtt_aligned_quad q = { };
        stbtt_GetBakedQuad(glyphs, 512, 512, (u8)text[i] - ASCII_START, &pos.x, &pos.y, &q, 1);

        r.batch[r.head * 4 + 0] = { Vec2(q.x0, q.y0), Vec2(q.s0, q.t0) };
        r.batch[r.head * 4 + 1] = { Vec2(q.x1, q.y0), Vec2(q.s1, q.t0) };
        r.batch[r.head * 4 + 2] = { Vec2(q.x1, q.y1), Vec2(q.s1, q.t1) };
        r.batch[r.head * 4 + 3] = { Vec2(q.x0, q.y1), Vec2(q.s0, q.t1) };

#if 0
        dbglog("%c: %fx%f", text[i], r.batch[r.head*4+2].p.x-r.batch[r.head*4+0].p.x, r.batch[r.head*4+2].p.y-r.batch[r.head*4+0].p.y);
#endif

        r.head += 1;
        if (r.head >= BATCH_SIZE) {
            flush_now = true;
            leftover = &text[i + 1];
            break;
        }
    }

    if (flush_now) {
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, r.vbo));
        GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * r.head * 4, r.batch));
        GLCHECK(glDrawElements(GL_TRIANGLES, r.head * 6, GL_UNSIGNED_INT, nullptr));
        r.head = 0;
    }

    if (leftover != nullptr) {
        DrawText(pos, leftover, flush);
    }
}

int main(int argc, const char* argv[]) {
    SDL_version sdlv_l; SDL_GetVersion(&sdlv_l);
    SDL_version sdlv_c; SDL_VERSION(&sdlv_c);
    dbglog("SDL v%d.%d.%d (compiled against v%d.%d.%d)",
        sdlv_l.major, sdlv_l.minor, sdlv_l.patch,
        sdlv_c.major, sdlv_c.minor, sdlv_c.patch);
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        dbgerr("Failed to initialize SDL: %s", SDL_GetError());
    }

    const u32 wndflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    SDL_Window* wnd = SDL_CreateWindow(__FILE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, wndflags);
    if (!wnd) {
        dbgerr("Failed to create SDL window: %s", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
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

    dbglog("GL_VERSION:  %s", glGetString(GL_VERSION));   GLCHECK();
    dbglog("GL_VENDOR:   %s", glGetString(GL_VENDOR));    GLCHECK();
    dbglog("GL_RENDERER: %s", glGetString(GL_RENDERER));  GLCHECK();

    // GLCHECK(glEnable(GL_DEPTH_TEST));
    GLCHECK(glEnable(GL_MULTISAMPLE));
    GLCHECK(glEnable(GL_BLEND));
    GLCHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    // GLCHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

    GLCHECK(glGenVertexArrays(1, &r.vao));
    GLCHECK(glBindVertexArray(r.vao));

    GLCHECK(glGenBuffers(1, &r.vbo));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, r.vbo));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * BATCH_SIZE * 4, nullptr, GL_DYNAMIC_DRAW));

    GLCHECK(glGenBuffers(1, &r.ibo));
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ibo));
    GLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * BATCH_SIZE * 6, nullptr, GL_STATIC_DRAW));
    u32* indices = (u32*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY); GLCHECK();
    for (usize i = 0; i < BATCH_SIZE; ++i) {
        indices[i * 6 + 0] = i * 4 + 0;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;
        indices[i * 6 + 3] = i * 4 + 0;
        indices[i * 6 + 4] = i * 4 + 2;
        indices[i * 6 + 5] = i * 4 + 3;
    }
    GLCHECK(glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER));

    GLCHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p)));
    GLCHECK(glEnableVertexAttribArray(0));
    GLCHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, t)));
    GLCHECK(glEnableVertexAttribArray(1));

    auto compile_shader = [](const char* vs_path, const char* fs_path) {
        GLuint prog = glCreateProgram(); GLCHECK();

        struct {
            GLenum type;
            const char* path;
        } shaders[] = {
            { GL_VERTEX_SHADER,     vs_path },
            { GL_FRAGMENT_SHADER,   fs_path },
        };
        for (usize i = 0; i < arrlen(shaders); ++i) {
            std::string source = load_text_file(shaders[i].path);
            if (source.empty()) {
                dbgerr("Failed to load %s", shaders[i].path);
            }
            const char* source_c = source.c_str();
            GLuint shader = glCreateShader(shaders[i].type); GLCHECK();
            GLCHECK(glShaderSource(shader, 1, (const GLchar**)&source_c, NULL));
            GLCHECK(glCompileShader(shader));
            GLint status = 0;
            GLCHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
            if (status != GL_TRUE) {
                char log[512];
                GLCHECK(glGetShaderInfoLog(shader, sizeof(log), NULL, log));
                dbgerr("Error while compiling shader %s: %s", shaders[i].path, log);
            }
            GLCHECK(glAttachShader(prog, shader));
        }
        GLCHECK(glLinkProgram(prog));
        GLint link_status = 0;
        GLCHECK(glGetProgramiv(prog, GL_LINK_STATUS, &link_status));
        if (link_status != GL_TRUE) {
            char log[512];
            GLCHECK(glGetProgramInfoLog(prog, sizeof(prog), NULL, log));
            dbgerr("Error while linking program: %s", log);
        }

        return prog;
    };

    // Compile shaders
    GLuint prog = compile_shader("opengl-text-vert.glsl", "opengl-text-frag.glsl");
    GLuint post = compile_shader("opengl-text-vert.glsl", "opengl-text-post-frag.glsl");

    std::vector<u8> ttf = load_binary_file("data/BerkeleyMono-Regular.ttf");
    if (ttf.empty()) {
       std::vector<u8> ttf = load_binary_file("data/sourcecodepro.ttf");
        if (ttf.empty()) {
            dbgerr("Failed to load TTF");
        } 
    }

    // 512x512 atlas
    std::vector<u8> pixels = std::vector<u8>(); pixels.resize(512 * 512);
    if (!stbtt_BakeFontBitmap(&ttf[0], 0, 32.0f, &pixels[0], 512, 512, ASCII_START, ASCII_SIZE, glyphs)) {
        dbgerr("Failed to create font bitmap");
    }
    GLuint tex; GLCHECK(glGenTextures(1, &tex));
    GLCHECK(glBindTexture(GL_TEXTURE_2D, tex));
    GLCHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]));
    GLCHECK(glGenerateMipmap(GL_TEXTURE_2D));

    // Frame buffer
    GLuint fbo; GLCHECK(glGenFramebuffers(1, &fbo));
    

    // Color attachment
    GLuint fbo_tex; GLCHECK(glGenTextures(1, &fbo_tex));

    int old_vp_w = -1;
    int old_vp_h = -1;

    SDL_ShowWindow(wnd);
    u8 wants_quit = 0;
    do {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
            case SDL_QUIT: {
                wants_quit = 1;
            } break;
            case SDL_KEYDOWN: {
                if (evt.key.keysym.sym == SDLK_q || evt.key.keysym.sym == SDLK_ESCAPE) {
                    wants_quit = 1;
                }
            } break;
            }
        }

        i32 vp_w = 0; i32 vp_h = 0;
        SDL_GL_GetDrawableSize(wnd, &vp_w, &vp_h);

        GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

        // Update framebuffer texture
        if (vp_w != old_vp_w || vp_h != old_vp_h) {
            old_vp_w = vp_w;
            old_vp_h = vp_h;

            GLCHECK(glBindTexture(GL_TEXTURE_2D, fbo_tex));
            GLCHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vp_w, vp_h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
            GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GLCHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0));
        }


        GLCHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
        GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GLCHECK(glViewport(0, 0, (f32)vp_w, (f32)vp_h));

        // vp_w /= 1.5;
        // vp_h /= 1.5;
        const Mat4 proj = Mat4::orthographic(0.1f, 2.0f, 0.0f, (f32)vp_w, 0.0f, (f32)vp_h);

        GLCHECK(glUseProgram(prog));
        GLCHECK(glUniform1i(glGetUniformLocation(prog, "u_atlas"), 0));
        GLCHECK(glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"), 1, GL_FALSE, proj.base()))

        // Sample font texture
        GLCHECK(glBindTexture(GL_TEXTURE_2D, tex));

        // Quad
#if 1
        const Vec2 p1 = Vec2((f32)vp_w, (f32)vp_h);
        const Vec2 p0 = p1 - Vec2(512.0f, 512.0f);
        
        Vertex quad[] = {
            { p0,               Vec2(0.0f, 1.0f) },
            { Vec2(p1.x, p0.y), Vec2(1.0f, 1.0f) },
            { p1,               Vec2(1.0f, 0.0f) },
            { Vec2(p0.x, p1.y), Vec2(0.0f, 0.0f) },
        };
        // dbglog("%fx%f, %dx%d", quad[2].p.x - quad[0].p.x, quad[2].p.y - quad[0].p.y, vp_w, vp_h);
        GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad));
        GLCHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
#endif

        const f32 now = SDL_GetTicks() / 1e3f;

        Vec2 cur = Vec2(20, 35);

        static auto push_text = [&](const char* text) {
            DrawText(cur, text);
            cur.y += 35.0f;
        };

        push_text("Hello, world!");
        push_text("The quick brown fox jumps over the lazy dog.");

        const char* animdemo = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas mollis vitae diam vitae cursus. ";
        char tmp[512] = { };
        strncpy(tmp, animdemo, (SDL_GetTicks() / 50) % strlen(animdemo));
        push_text(tmp);

        char loaddemo[64] = { };
        for (usize i = 0; i < arrlen(loaddemo) - 1; ++i) {
            char* c = &loaddemo[i];
            switch (i) {
            case 0: {
                *c = '[';
            } break;
            case arrlen(loaddemo) - 2: {
                *c = ']';
            } break;
            default: {
                *c = (i > (SDL_GetTicks() / 25) % (arrlen(loaddemo) - 2)) ? '-' : '0';
            }
            }
        }
        push_text(loaddemo);

        DrawText(Vec2((sin(now) / 4.0f + 0.5f) * vp_w, (cos(now) / 4.0f + 0.5f) * vp_h), "WOW!");

        DrawText(Vec2(), nullptr, true);

        // Draw post-processing
        GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCHECK(glUseProgram(post));
        GLCHECK(glUniform1i(glGetUniformLocation(post, "u_atlas"), 0));
        GLCHECK(glUniformMatrix4fv(glGetUniformLocation(post, "u_proj"), 1, GL_FALSE, proj.base()))
        GLCHECK(glUniform2f(glGetUniformLocation(post, "u_vp"), (f32)vp_w, (f32)vp_h));
        GLCHECK(glBindTexture(GL_TEXTURE_2D, fbo_tex));

        GLCHECK(glClear(GL_COLOR_BUFFER_BIT));

        const Vec2 fb_p0 = Vec2(0.0f, 0.0f);
        const Vec2 fb_p1 = Vec2((f32)vp_w, (f32)vp_h);
        Vertex fb_quad[] = {
            { fb_p0,                    Vec2(0.0f, 1.0f) },
            { Vec2(fb_p1.x, fb_p0.y),   Vec2(1.0f, 1.0f) },
            { fb_p1,                    Vec2(1.0f, 0.0f) },
            { Vec2(fb_p0.x, fb_p1.y),   Vec2(0.0f, 0.0f) },
        };
        GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fb_quad), fb_quad));
        GLCHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

        SDL_GL_SwapWindow(wnd);
    } while (!wants_quit);

    SDL_DestroyWindow(wnd);

    return EXIT_SUCCESS;

}