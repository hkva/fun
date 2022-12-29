#include "engine.h"

#include <GL/glew.h>
#include "SDL.h"

enum {
    _E_STATE_WANTS_QUIT = 1 << 0,
};

enum {
    _E_PROG_2D,     // flat textured 2D
    _E_PROG_COUNT,
};

enum {
    _E_TEX_WHITE,
    _E_TEX_COUNT,
};

typedef struct _E_Vert {
    float p[2];
    float t[2];
    float c[4];
} _E_Vert;

#define _E_BATCH_SIZE 64

// Game engine state
static struct {
    // Application state
    SDL_Window*     window;
    SDL_GLContext   gl;
    uint32_t        state;
    // Rendering state
    GLsizei         vp_w, vp_h;
    GLuint          progs[_E_PROG_COUNT];
    GLuint          tex_builtin[_E_TEX_COUNT];
    uint32_t        color;
    // Batch triangle renderer
    GLuint          b_texture;
    _E_Vert         b_batch[_E_BATCH_SIZE * 3];
    GLuint          b_vao, b_vbo;
    unsigned int    b_count;
} _e;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Core
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static void GLAPIENTRY _e_glerr(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    fun_msg("GL: %s", message);
}

static GLuint _e_compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    // check compile status
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        static char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fun_err("Failed to compile OpenGL shader: %s", log);
    }

    return shader;
}

static GLuint _e_compile_prog(const char* v_src, const char* f_src) {
    GLuint program = glCreateProgram();
    GLuint vs = _e_compile_shader(GL_VERTEX_SHADER, v_src);
    GLuint fs = _e_compile_shader(GL_FRAGMENT_SHADER, f_src);
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        static char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fun_err("Failed to link OpenGL program: %s", log);
    }
    return program;
}

void e_create(const char* window_title) {
    FUN_ZERO(_e);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fun_err("Failed to initialize SDL: %s", SDL_GetError());
    }
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    if ((_e.window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, flags)) == NULL) {
        fun_err("Failed to create SDL window: %s", SDL_GetError());
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    if ((_e.gl = SDL_GL_CreateContext(_e.window)) == NULL) {
        fun_err("Failed to create OpenGL context: %s", SDL_GetError());
    }

    // Load OpenGL functions
    if (glewInit() != GLEW_OK) {
        fun_err("Failed to load OpenGL functions");
    }

    // Enable debug output
    glDebugMessageCallback(_e_glerr, NULL);
    glEnable(GL_DEBUG_OUTPUT);

    // Compile programs
    const char* prog_2d_vs =
        "#version 330 core                                  \n"
        "                                                   \n"
        "layout (location = 0) in vec2 p;                   \n"
        "layout (location = 1) in vec2 t;                   \n"
        "layout (location = 2) in vec4 c;                   \n"
        "                                                   \n"
        "out vec2 v_t;                                      \n"
        "out vec4 v_c;                                      \n"
        "                                                   \n"
        "uniform mat4 u_proj;                               \n"
        "                                                   \n"
        "void main() {                                      \n"
        "    v_t = t;                                       \n"
        "    v_c = c;                                       \n"
        "    gl_Position = u_proj * vec4(p, -1.0f, 1.0f);   \n"
        "}                                                  \n";
    const char* prog_2d_fs =
        "#version 330 core                                  \n"
        "                                                   \n"
        "in vec2 v_t;                                       \n"
        "in vec4 v_c;                                       \n"
        "                                                   \n"
        "out vec4 f_clr;                                    \n"
        "                                                   \n"
        "void main() {                                      \n"
        "    f_clr = v_c;                                   \n"
        "}                                                  \n";
    _e.progs[_E_PROG_2D] = _e_compile_prog(prog_2d_vs, prog_2d_fs);

    // Create built-in textures
    uint32_t white_pixels[8*8];
    memset(white_pixels, 0xFF, sizeof(white_pixels));
    E_Image white = { 8, 8, white_pixels };
    _e.tex_builtin[_E_TEX_WHITE] = e_create_texture(&white);

    // Initialize the batch renderer
    _e.b_texture = _e.tex_builtin[_E_TEX_WHITE];
    // vao
    glGenVertexArrays(1, &_e.b_vao);
    glBindVertexArray(_e.b_vao);
    // vbo
    glGenBuffers(1, &_e.b_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _e.b_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(_e.b_batch), NULL, GL_DYNAMIC_DRAW);
    // layout
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(_E_Vert), (void*)(sizeof(float) * 0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(_E_Vert), (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(_E_Vert), (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(2);

    e_draw_set_color(E_CWHITE);
}

void e_destroy() {
    SDL_DestroyWindow(_e.window);
    SDL_Quit();
}

void e_poll_events() {
    SDL_Event evt;
    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_QUIT: {
                _e.state |= _E_STATE_WANTS_QUIT;
            } break;
        };
    }
}

bool e_wants_quit(void) {
    return !!(_e.state & _E_STATE_WANTS_QUIT);
}

void e_present(void) {
    SDL_GL_SwapWindow(_e.window);
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Time
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

float e_now(void) {
    return SDL_GetTicks() / 1e3f;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Asset processing
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

E_Texture e_create_texture(const E_Image* img) {
    E_Texture tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);
    glGenerateMipmap(tex);
    return tex;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Rendering
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static void _e_flush_renderer(void) {
    if (_e.b_count == 0) {
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, _e.b_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_e.b_batch), _e.b_batch);
    glDrawArrays(GL_TRIANGLES, 0, _e.b_count);
    _e.b_count = 0;
}

static void _e_push_triangle(float* restrict p, float* restrict t, float* restrict c) {
    if (_e.b_count + 3 >= FUN_ARRLEN(_e.b_batch)) {
        _e_flush_renderer();
    }
    for (unsigned int i = 0; i < 3; ++i) {
        // p
        _e.b_batch[_e.b_count].p[0] = p[i*2+0];
        _e.b_batch[_e.b_count].p[1] = p[i*2+1];
        // t
        _e.b_batch[_e.b_count].t[0] = t[i*2+0];
        _e.b_batch[_e.b_count].t[1] = t[i*2+1];
        // c
        _e.b_batch[_e.b_count].c[0] = c[i*4+0];
        _e.b_batch[_e.b_count].c[1] = c[i*4+1];
        _e.b_batch[_e.b_count].c[2] = c[i*4+2];
        _e.b_batch[_e.b_count].c[3] = c[i*4+3];

        _e.b_count += 1;
    }
}

void e_begin_frame(void) {
    SDL_GL_GetDrawableSize(_e.window, &_e.vp_w, &_e.vp_h);
    glViewport(0, 0, _e.vp_w, _e.vp_h);
    // Calculate orthographic projection matrix
    float proj[4*4];
    fun_mat4_orthographic(proj, 0, (float)_e.vp_w, (float)_e.vp_h, 0, 0.1f, 2.0f);
    // Set up shader
    glUseProgram(_e.progs[_E_PROG_2D]);
    glUniformMatrix4fv(glGetUniformLocation(_e.progs[_E_PROG_2D], "u_proj"), 1, GL_FALSE, proj);
}

void e_end_frame(void) {
    _e_flush_renderer();
}

void e_clear(uint32_t clr) {
    glClearColor(E_REDF(clr), E_GREENF(clr), E_BLUEF(clr), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void e_draw_set_color(uint32_t clr) {
    _e.color = clr;
}

void e_draw_triangle(float* restrict points) {
    float t[2*3] = { 0 };
    float c[4*3] = {
        E_REDF(_e.color), E_GREENF(_e.color), E_BLUEF(_e.color), E_ALPHAF(_e.color),
        E_REDF(_e.color), E_GREENF(_e.color), E_BLUEF(_e.color), E_ALPHAF(_e.color),
        E_REDF(_e.color), E_GREENF(_e.color), E_BLUEF(_e.color), E_ALPHAF(_e.color),
    };
    _e_push_triangle(points, t, c);
}

void e_draw_line(float x1, float y1, float x2, float y2) {
    e_draw_line_ex(x1, y1, x2, y2, 1.0f);
}

void e_draw_line_ex(float x1, float y1, float x2, float y2, float width) {
    float dx = x2 - x1; float dy = y2 - y1; // direction vector
    float len2 = dx * dx + dy * dy;
    if (len2 == 0.0f) { return; }
    float len = sqrtf(len2);
    dx /= len; dy /= len;
    float updir_x = -dy; float updir_y = dx;
    float halfwidth = width / 2.0f;
    float x1up = x1 + updir_x * halfwidth; float y1up = y1 + updir_y * halfwidth;
    float x2up = x2 + updir_x * halfwidth; float y2up = y2 + updir_y * halfwidth;
    float x1dn = x1 - updir_x * halfwidth; float y1dn = y1 - updir_y * halfwidth;
    float x2dn = x2 - updir_x * halfwidth; float y2dn = y2 - updir_y * halfwidth;
    float tri1[2*3] = {
        x1up, y1up,
        x2up, y2up,
        x1dn, y1dn,
    };
    float tri2[2*3] = {
        x1dn, y1dn,
        x2up, y2up,
        x2dn, y2dn,
    };
    e_draw_triangle(tri1); e_draw_triangle(tri2);
}

