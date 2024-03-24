// SPDX-License-Identifier: MIT

#define DEMO_NAME "Text"
#include "opengl.hh"

#include "stb_truetype.h"

const char* VS = R"""(
#version 330 core

layout (location = 0) in vec2 p;
layout (location = 1) in vec2 t;

uniform mat4 u_proj;

out vec2 v_t;

void main() {
    v_t = t;
    gl_Position = vec4(vec3(p, -1.0f), 1.0f) * u_proj;
}
)""";

const char* FS = R"""(
#version 330 core

in vec2 v_t;

out vec4 color;

uniform sampler2D u_atlas;

void main() {
    color = vec4(1.0f, 0.67f, 0.27f, texture(u_atlas, v_t).r);
    // color = vec4(vec3(0.85f), texture(u_atlas, v_t).r);
    // color = vec4(1.0f);
}
)""";

const char* FS_POST = R"""(
#version 330 core

in vec2 v_t;

out vec4 color;

uniform sampler2D u_atlas;
uniform vec2 u_vp;

void main() {
#if 0
    vec2 cabbr_r = vec2(-1.0f, 0.0f);
    vec2 cabbr_g = vec2(0.0f, 0.0f);
    vec2 cabbr_b = vec2(1.0f, 0.0f);
#else
    vec2 cabbr_r = vec2(0.0f, 0.0f);
    vec2 cabbr_g = vec2(0.0f, 0.0f);
    vec2 cabbr_b = vec2(0.0f, 0.0f);
#endif

    float r = texture(u_atlas, v_t + (cabbr_r / u_vp)).r;
    float g = texture(u_atlas, v_t + (cabbr_g / u_vp)).g;
    float b = texture(u_atlas, v_t + (cabbr_b / u_vp)).b; 

    color = vec4(r, g, b, 1.0f);
}
)""";

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

static GLuint prog = 0;
static GLuint post = 0;

static Vec2 old_vp = Vec2(-1.0f, -1.0f);

static GLuint tex;
static GLuint fbo;
static GLuint fbo_tex;

static u32 num_chars = 0;
static u32 num_draws = 0;

static void draw_text(Vec2 pos, const char* text, bool flush = false) {
    bool flush_now = flush;
    const char* leftover = nullptr; // text left over after flush
    for (usize i = 0; text && text[i]; ++i) {
        if ((u8)text[i] < ASCII_START || (u8)text[i] > ASCII_END) {
            continue;
        }

        ++num_chars;

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
        glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * r.head * 4, r.batch);
        glDrawElements(GL_TRIANGLES, r.head * 6, GL_UNSIGNED_INT, nullptr);
        ++num_draws;
        r.head = 0;
    }

    if (leftover != nullptr) {
        draw_text(pos, leftover, flush);
    }
}

void demo_init(const App* app) {
    old_vp = Vec2(-1.0f, -1.0f);

    // Disable depth testing
    glDisable(GL_DEPTH_TEST);

    // Enable multisampling
    glEnable(GL_MULTISAMPLE);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &r.vao);
    glBindVertexArray(r.vao);

    glGenBuffers(1, &r.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * BATCH_SIZE * 4, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &r.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * BATCH_SIZE * 6, nullptr, GL_STATIC_DRAW);
    u32* indices = (u32*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);;
    for (usize i = 0; i < BATCH_SIZE; ++i) {
        indices[i * 6 + 0] = i * 4 + 0;
        indices[i * 6 + 1] = i * 4 + 1;
        indices[i * 6 + 2] = i * 4 + 2;
        indices[i * 6 + 3] = i * 4 + 0;
        indices[i * 6 + 4] = i * 4 + 2;
        indices[i * 6 + 5] = i * 4 + 3;
    }
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, t));
    glEnableVertexAttribArray(1);

    prog = compile_gl_program(VS, FS);
    post = compile_gl_program(VS, FS_POST);

    std::vector<u8> ttf = load_binary_file("data/BerkeleyMono-Regular.ttf");
    if (ttf.empty()) {
       ttf = load_binary_file("data/sourcecodepro.ttf");
        if (ttf.empty()) {
            dbgerr("Failed to load TTF");
        } 
    }

    // 512x512 atlas
    std::vector<u8> pixels = std::vector<u8>(); pixels.resize(512 * 512);
    if (!stbtt_BakeFontBitmap(&ttf[0], 0, 32.0f, &pixels[0], 512, 512, ASCII_START, ASCII_SIZE, glyphs)) {
        dbgerr("Failed to create font bitmap");
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Frame buffer
    glGenFramebuffers(1, &fbo);

    // Color attachment
    glGenTextures(1, &fbo_tex);
}

void demo_frame(const App* app) {
    num_chars = 0; num_draws = 0;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Update framebuffer texture
    if (app->vp.x != old_vp.x || app->vp.y != old_vp.y) {
        old_vp = app->vp;

        glBindTexture(GL_TEXTURE_2D, fbo_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, app->vp.x, app->vp.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
    }

    glViewport(0, 0, app->vp.x, app->vp.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const Mat4 proj = Mat4::orthographic(0.1f, 2.0f, 0.0f, app->vp.x, 0.0f, app->vp.y);

    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "u_atlas"), 0);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"), 1, GL_FALSE, proj.base());

    // Sample font texture
    glBindTexture(GL_TEXTURE_2D, tex);

    // Quad
#if 1
    const Vec2 p1 = app->vp;
    const Vec2 p0 = p1 - Vec2(512.0f, 512.0f);
        
    Vertex quad[] = {
        { p0,               Vec2(0.0f, 1.0f) },
        { Vec2(p1.x, p0.y), Vec2(1.0f, 1.0f) },
        { p1,               Vec2(1.0f, 0.0f) },
        { Vec2(p0.x, p1.y), Vec2(0.0f, 0.0f) },
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
#endif

    const f32 now = SDL_GetTicks() / 1e3f;

    Vec2 cur = Vec2(20, 100);

    static auto push_text = [&](const char* text) {
        draw_text(cur, text);
        cur.y += 35.0f;
    };

    push_text("Hello, world!");
    push_text("The quick brown fox jumps over the lazy dog.");

    const char* animdemo = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas mollis vitae diam vitae cursus. ";
    char tmp[512] = { };
    strncpy(tmp, animdemo, (SDL_GetTicks() / 100) % strlen(animdemo));
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

    const char* helloworld = "ASCII STANDS FOR AMERICAN STANDARD CODE FOR INFORMATION INTERCHANGE";
    for (usize i = 0; helloworld[i] != '\0'; ++i) {
        char text[2] = { helloworld[i], '\0' };
        f32 off = (f32)i / 15.0f;
        draw_text(Vec2((sin(now - off) / 4.0f + 0.5f) * app->vp.x, (cos(now - off) / 4.0f + 0.5f) * app->vp.y), text);
    }

    // draw_text(Vec2((sin(now) / 4.0f + 0.5f) * app->vp.x, (cos(now) / 4.0f + 0.5f) * app->vp.y), "WOW!");

    draw_text(Vec2(), nullptr, true);

    push_text("Random stuff:");
    const u32 rows = 11;
    const u32 cols = 75;
    RandomXOR r = RandomXOR();
    const u32 t = (SDL_GetTicks() / 10) % (rows * cols);
    for (u32 i = 0; i < (t / cols) + 1; ++i) {
        char buf[128] = { };
        for (u32 j = 0; j < min(cols, t - (i * cols)); ++j) {
            buf[j] = r.random<char>(' ', '~');
        }
        push_text(buf);
    }

    // Draw post-processing
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(post);
    glUniform1i(glGetUniformLocation(post, "u_atlas"), 0);
    glUniformMatrix4fv(glGetUniformLocation(post, "u_proj"), 1, GL_FALSE, proj.base());
    glUniform2f(glGetUniformLocation(post, "u_vp"), app->vp.x, app->vp.y);
    glBindTexture(GL_TEXTURE_2D, fbo_tex);

    glClear(GL_COLOR_BUFFER_BIT);

    const Vec2 fb_p0 = Vec2(0.0f, 0.0f);
    const Vec2 fb_p1 = app->vp;
    Vertex fb_quad[] = {
        { fb_p0,                    Vec2(0.0f, 1.0f) },
        { Vec2(fb_p1.x, fb_p0.y),   Vec2(1.0f, 1.0f) },
        { fb_p1,                    Vec2(1.0f, 0.0f) },
        { Vec2(fb_p0.x, fb_p1.y),   Vec2(0.0f, 0.0f) },
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fb_quad), fb_quad);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void demo_ui(const App* app) {
    ImGui::Text("# glyphs: %u", num_chars);
    ImGui::Text("# draws: %u (%u / %u)", num_draws, num_chars, (u32)BATCH_SIZE);
    float dpi = 0.0f; SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
    ImGui::Text("DPI: %f", dpi);
    ImGui::Text("DPI / 96.0f: %f", dpi / 96.0f);
    ImGui::Text("DPI / 72.0f: %f", dpi / 72.0f);
}
