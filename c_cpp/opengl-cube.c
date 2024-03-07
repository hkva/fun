// SPDX-License-Identifier: MIT

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <glad/glad.h>

#include "util/common.h"
#include "util/math.h"

typedef struct Vertex {
    V3 p;
    V2 t;
} Vertex;

typedef struct Mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
} Mesh;

enum {
    CAM_PERSPECTIVE,
    CAM_ORTHOGRAPHIC,
};

typedef struct Camera {
    int type;
    V3 pos;
    V3 ang;
    F32 fov;
    F32 near;
    F32 far;
    F32 aspect;
} Camera;

typedef struct Model {
    Mesh mesh;
    M4x4 transform;
    F32 color[3];
    F32 grid_scale;
    GLuint prog;
    USize num_indices;
} Model;

#if 1
    #define GLCHECK(code) do {                                                                          \
        code;                                                                                           \
        const U32 glcheck_err = glGetError();                                                           \
        if (glcheck_err != GL_NO_ERROR) {                                                               \
            Error("%s:%d: %s -> %s", __FILE__, __LINE__, #code, GetGLErrorName(glcheck_err));           \
        }                                                                                               \
    } while (0);
#else
    #define GLCHECK(code) code
#endif

static inline const char* GetGLErrorName(GLenum error) {
    switch (error) {
    case GL_NO_ERROR:                       return "GL_NO_ERROR";
    case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
    default:                                return "(unknown error)";
    };
};

static void DrawScene(const Model* models, USize num_models, Camera cam) {
    M4x4 cam_proj = Mat4Perspective(cam.near, cam.far, cam.aspect, cam.fov);

    M4x4 cam_transform = Mat4Translate(Vec3Invert(cam.pos));

    for (USize i = 0; i < num_models; ++i) {
        const Model* m = &models[i];

        M4x4 model_transform = Mat4Mul(m->transform, cam_transform);

        GLCHECK(glBindVertexArray(m->mesh.vao));

        GLCHECK(glUseProgram(m->prog));

        GLCHECK(glUniformMatrix4fv(glGetUniformLocation(m->prog, "u_model"), 1, GL_FALSE, model_transform.m));
        GLCHECK(glUniformMatrix4fv(glGetUniformLocation(m->prog, "u_proj"), 1, GL_FALSE, cam_proj.m));

        GLCHECK(glUniform1f(glGetUniformLocation(m->prog, "u_grid_scale"), m->grid_scale));
        GLCHECK(glUniform4f(glGetUniformLocation(m->prog, "u_color"), m->color[0], m->color[1], m->color[2], 1.0f));

        GLCHECK(glDrawElements(GL_TRIANGLES, m->num_indices, GL_UNSIGNED_INT, NULL));
    }
}

int main(int argc, const char* argv[]) {
    SDL_version sdlv_l; SDL_GetVersion(&sdlv_l);
    SDL_version sdlv_c; SDL_VERSION(&sdlv_c);
    Info("SDL v%d.%d.%d (compiled against v%d.%d.%d)",
        sdlv_l.major, sdlv_l.minor, sdlv_l.patch,
        sdlv_c.major, sdlv_c.minor, sdlv_c.patch);
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        Error("Failed to initialize SDL: %s", SDL_GetError());
    }

    const U32 wndflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    SDL_Window* wnd = SDL_CreateWindow(__FILE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, wndflags);
    if (!wnd) {
        Error("Failed to create SDL window: %s", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
    SDL_GLContext gl = SDL_GL_CreateContext(wnd);
    if (!gl) {
        Error("Failed to create OpenGL context: %s", SDL_GetError());
    }
    if (SDL_GL_MakeCurrent(wnd, gl) < 0) {
        Error("Failed to activate OpenGL context: %s", SDL_GetError());
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        Error("Failed to load OpenGL functions");
    }

    Info("GL_VERSION:  %s", glGetString(GL_VERSION));   GLCHECK();
    Info("GL_VENDOR:   %s", glGetString(GL_VENDOR));    GLCHECK();
    Info("GL_RENDERER: %s", glGetString(GL_RENDERER));  GLCHECK();

    // Enable depth testing
    GLCHECK(glEnable(GL_DEPTH_TEST));

    // Enable multisamplgin
    GLCHECK(glEnable(GL_MULTISAMPLE));

     // Compile shaders
    GLuint prog = glCreateProgram(); GLCHECK();

    struct {
        GLenum type;
        const char* path;
    } shaders[] = {
        { GL_VERTEX_SHADER,     "opengl-cube-vert.glsl" },
        { GL_FRAGMENT_SHADER,   "opengl-cube-frag.glsl" },
    };
    for (USize i = 0; i < ArrLen(shaders); ++i) {
        char* source = LoadTextFile(shaders[i].path);
        if (!source) {
            Error("Failed to load %s", shaders[i].path);
        }
        GLuint shader = glCreateShader(shaders[i].type); GLCHECK();
        GLCHECK(glShaderSource(shader, 1, (const GLchar**)&source, NULL));
        GLCHECK(glCompileShader(shader));
        GLint status = 0;
        GLCHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
        if (status != GL_TRUE) {
            char log[512];
            GLCHECK(glGetShaderInfoLog(shader, sizeof(log), NULL, log));
            Error("Error while compiling shader %s: %s", shaders[i].path, log);
        }
        GLCHECK(glAttachShader(prog, shader));
    }
    GLCHECK(glLinkProgram(prog));
    GLint link_status = 0;
    GLCHECK(glGetProgramiv(prog, GL_LINK_STATUS, &link_status));
    if (link_status != GL_TRUE) {
        char log[512];
        GLCHECK(glGetProgramInfoLog(prog, sizeof(prog), NULL, log));
        Error("Error while linking program: %s", log);
    }

    //    +-----------+
    //   /|          /|
    //  / |         / |
    // +-----------+  |
    // |  |        |  |
    // |  |        |  |
    // |  +--------|--+
    // | /         | /
    // |/          |/
    // +-----------+

    const Vertex cube_verts[] = {
        // Back face
        { Vec3( 0.5f,  0.5f, -0.5f), Vec2(0.0f, 1.0f) },
        { Vec3(-0.5f,  0.5f, -0.5f), Vec2(1.0f, 1.0f) },
        { Vec3(-0.5f, -0.5f, -0.5f), Vec2(1.0f, 0.0f) },
        { Vec3( 0.5f, -0.5f, -0.5f), Vec2(0.0f, 0.0f) },
        // Left face
        { Vec3(-0.5f,  0.5f, -0.5f), Vec2(0.0f, 1.0f) },
        { Vec3(-0.5f,  0.5f,  0.5f), Vec2(1.0f, 1.0f) },
        { Vec3(-0.5f, -0.5f,  0.5f), Vec2(1.0f, 0.0f) },
        { Vec3(-0.5f, -0.5f, -0.5f), Vec2(0.0f, 0.0f) },
        // Front face
        { Vec3(-0.5f,  0.5f,  0.5f), Vec2(0.0f, 1.0f) },
        { Vec3( 0.5f,  0.5f,  0.5f), Vec2(1.0f, 1.0f) },
        { Vec3( 0.5f, -0.5f,  0.5f), Vec2(1.0f, 0.0f) },
        { Vec3(-0.5f, -0.5f,  0.5f), Vec2(0.0f, 0.0f) },
        // Right face
        { Vec3( 0.5f,  0.5f,  0.5f), Vec2(0.0f, 1.0f) },
        { Vec3( 0.5f,  0.5f, -0.5f), Vec2(1.0f, 1.0f) },
        { Vec3( 0.5f, -0.5f, -0.5f), Vec2(1.0f, 0.0f) },
        { Vec3( 0.5f, -0.5f,  0.5f), Vec2(0.0f, 0.0f) },
        // Top face
        { Vec3(-0.5f,  0.5f, -0.5f), Vec2(0.0f, 1.0f) },
        { Vec3( 0.5f,  0.5f, -0.5f), Vec2(1.0f, 1.0f) },
        { Vec3( 0.5f,  0.5f,  0.5f), Vec2(1.0f, 0.0f) },
        { Vec3(-0.5f,  0.5f,  0.5f), Vec2(0.0f, 0.0f) },
        // Bottom face
        { Vec3(-0.5f, -0.5f,  0.5f), Vec2(0.0f, 1.0f) },
        { Vec3( 0.5f, -0.5f,  0.5f), Vec2(1.0f, 1.0f) },
        { Vec3( 0.5f, -0.5f, -0.5f), Vec2(1.0f, 0.0f) },
        { Vec3(-0.5f, -0.5f, -0.5f), Vec2(0.0f, 0.0f) },
    };

    const U32 cube_indices[] = {
         0,  1,  2,
         0,  2,  3,
         4,  5,  6,
         4,  6,  7,
         8,  9, 10,
         8, 10, 11,
        12, 13, 14,
        12, 14, 15,
        16, 17, 18,
        16, 18, 19,
        20, 21, 22,
        20, 22, 23,
    };

    Model cube = { 0 };
    cube.color[0] = 0.3f; cube.color[1] = 0.3f; cube.color[2] = 0.6f;
    cube.grid_scale = 8.0f;
    cube.prog = prog;
    cube.num_indices = ArrLen(cube_indices);

    // Vertex array object - stores array bindings
    GLCHECK(glGenVertexArrays(1, &cube.mesh.vao));
    GLCHECK(glBindVertexArray(cube.mesh.vao));

    // Vertex buffer object - stores vertex data
    GLCHECK(glGenBuffers(1, &cube.mesh.vbo));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, cube.mesh.vbo));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW));

    // Index buffer object - stores index data
    GLCHECK(glGenBuffers(1, &cube.mesh.ibo));
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.mesh.ibo));
    GLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW));

    // Set vertex layout
    GLCHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p)));
    GLCHECK(glEnableVertexAttribArray(0));
    GLCHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, t)));
    GLCHECK(glEnableVertexAttribArray(1));

    // +-----------+
    // |           |
    // |           |
    // |           |
    // |           |
    // |           |
    // +-----------+

    const Vertex plane_verts[] = {
        { Vec3(-0.5f,  0.5f,  0.0f), Vec2(0.0f, 1.0f) },
        { Vec3( 0.5f,  0.5f,  0.0f), Vec2(1.0f, 1.0f) },
        { Vec3( 0.5f, -0.5f,  0.0f), Vec2(1.0f, 0.0f) },
        { Vec3(-0.5f, -0.5f,  0.0f), Vec2(0.0f, 0.0f) },
    };

    const U32 plane_indices[] = {
         0,  1,  2,
         0,  2,  3,
    };

    Model plane = { 0 };
    plane.color[0] = 0.4f; plane.color[1] = 0.4f; plane.color[2] = 0.4f;
    plane.grid_scale = 8.0f;
    plane.prog = prog;
    plane.num_indices = ArrLen(plane_indices);

    GLCHECK(glGenVertexArrays(1, &plane.mesh.vao));
    GLCHECK(glBindVertexArray(plane.mesh.vao));

    GLCHECK(glGenBuffers(1, &plane.mesh.vbo));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, plane.mesh.vbo));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(plane_verts), plane_verts, GL_STATIC_DRAW));

    GLCHECK(glGenBuffers(1, &plane.mesh.ibo));
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane.mesh.ibo));
    GLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_indices), plane_indices, GL_STATIC_DRAW));

    GLCHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p)));
    GLCHECK(glEnableVertexAttribArray(0));
    GLCHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, t)));
    GLCHECK(glEnableVertexAttribArray(1));

    SDL_ShowWindow(wnd);
    U8 wants_quit = 0;
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

        //
        // Update scene
        //

        const F32 t = (F32)SDL_GetTicks() / 1e3f;

        // r: <t * 30.0f, t * 30.0f, 0.0f>
        cube.transform = Mat4Mul(
            Mat4RotateX(t * 30.0f),
            Mat4RotateY(t * 30.0f)
        );

        // p: <0.0f, -1.0f, 0.0>
        // r: <90.0f, 0.0f, 0.0f>
        // s: <4.0f, 4.0f, 4.0f>
        plane.transform = Mat4Mul(
            Mat4RotateX(90.0f), 
            Mat4Mul(
                Mat4ScaleUniform(4.0f),
                Mat4Translate(Vec3(0.0f, -1.0f, 0.0f))
            )
        );

        S32 vp_w = 0; S32 vp_h = 0;
        SDL_GL_GetDrawableSize(wnd, &vp_w, &vp_h);

        GLCHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
        GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GLCHECK(glViewport(0, 0, (F32)vp_w, (F32)vp_h));

        Model scene[] = { cube, plane };

        //
        // Draw scene
        //

        Camera cam = { 0 };
        cam.pos = Vec3(0.0f, 0.0f, 1.5f);
        cam.type = CAM_PERSPECTIVE;
        cam.fov = 90.0f;
        cam.near = 0.1f;
        cam.far = 20.0f;
        cam.aspect = (F32)vp_w / (F32)vp_h;

        DrawScene(scene, ArrLen(scene), cam);

        SDL_GL_SwapWindow(wnd);
    } while (!wants_quit);

    SDL_DestroyWindow(wnd);

    return EXIT_SUCCESS;
}