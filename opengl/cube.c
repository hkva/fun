#include "fun.h"

#include <GL/glew.h>
#include <SDL.h>

// Vertex shader
static const char* CUBE_VS =
    "#version 330 core                                  \n"
    "                                                   \n"
    "layout (location = 0) in vec3 p;                   \n"
    "layout (location = 1) in vec3 n;                   \n"
    "layout (location = 2) in vec2 t;                   \n"
    "                                                   \n"
    "uniform mat4 u_proj;                               \n"
    "uniform mat4 u_model;                              \n"
    "                                                   \n"
    "out vec2 v_t;                                      \n"
    "                                                   \n"
    "void main() {                                      \n"
    "   v_t = t;                                        \n"
    "   gl_Position = u_proj * u_model * vec4(p, 1.0f); \n"
    "}                                                  \n";

// Fragment shader
static const char* CUBE_FS =
    "#version 330 core                                                              \n"
    "                                                                               \n"
    "in vec2 v_t;                                                                   \n"
    "                                                                               \n"
    "out vec4 color;                                                                \n"
    "                                                                               \n"
    "void main() {                                                                  \n"
    "   int checker = 16;                                                           \n"
    "   if ((int(v_t.x * checker) % 2 == 0) ^^ (int(v_t.y * checker) % 2 == 0)) {   \n"
    "       color = vec4(0.47f, 0.55f, 0.81f, 1.0f);                                \n"
    "   } else {                                                                    \n"
    "       color = vec4(0.29f, 0.38f, 0.68f, 1.0f);                                \n"
    "   }                                                                           \n"
    "}                                                                              \n";

// Cube vertex data
static const float CUBE_VERTICES[] = {
    // position             normal          texcoord
    -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,    1.0f, 0.0f, 0.0f, 
    -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,    1.0f, 0.0f, 1.0f, 
     0.5f,  0.5f,  0.5f,    0.0f,  0.0f,    1.0f, 1.0f, 1.0f, 
     0.5f, -0.5f,  0.5f,    0.0f,  0.0f,    1.0f, 1.0f, 0.0f, 
    // left face
    -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,    0.0f, 0.0f, 0.0f, 
    -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,    0.0f, 0.0f, 1.0f, 
    -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,    0.0f, 1.0f, 1.0f, 
    -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,    0.0f, 1.0f, 0.0f, 
    // back face
     0.5f, -0.5f, -0.5f,    0.0f,  0.0f,   -1.0f, 0.0f, 0.0f, 
     0.5f,  0.5f, -0.5f,    0.0f,  0.0f,   -1.0f, 0.0f, 1.0f, 
    -0.5f,  0.5f, -0.5f,    0.0f,  0.0f,   -1.0f, 1.0f, 1.0f, 
    -0.5f, -0.5f, -0.5f,    0.0f,  0.0f,   -1.0f, 1.0f, 0.0f, 
    // right face
     0.5f, -0.5f,  0.5f,    1.0f,  0.0f,    0.0f, 0.0f, 0.0f, 
     0.5f,  0.5f,  0.5f,    1.0f,  0.0f,    0.0f, 0.0f, 1.0f, 
     0.5f,  0.5f, -0.5f,    1.0f,  0.0f,    0.0f, 1.0f, 1.0f, 
     0.5f, -0.5f, -0.5f,    1.0f,  0.0f,    0.0f, 1.0f, 0.0f, 
    // top face
    -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,    0.0f, 0.0f, 0.0f, 
    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,    0.0f, 0.0f, 1.0f, 
     0.5f,  0.5f, -0.5f,    0.0f,  1.0f,    0.0f, 1.0f, 1.0f, 
     0.5f,  0.5f,  0.5f,    0.0f,  1.0f,    0.0f, 1.0f, 0.0f, 
    // bottom face
     0.5f, -0.5f,  0.5f,    0.0f, -1.0f,    0.0f, 0.0f, 0.0f, 
     0.5f, -0.5f, -0.5f,    0.0f, -1.0f,    0.0f, 0.0f, 1.0f, 
    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,    0.0f, 1.0f, 1.0f, 
    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,    0.0f, 1.0f, 0.0f, 
};

// Cube index data
static const unsigned int CUBE_INDICES[] = {
    // front face
    0, 1, 2,
    0, 2, 3,
    // left face
    4, 5, 6,
    4, 6, 7,
    // back face
    8, 9, 10,
    8, 10, 11,
    // right face
    12, 13, 14,
    12, 14, 15,
    // top face
    16, 17, 18,
    16, 18, 19,
    // bottom face
    20, 21, 22,
    20, 22, 23,
};

static GLuint compile_shader(const char* src, GLenum type) {
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

static void GLAPIENTRY on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    fun_msg("GL: %s", message);
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fun_err("Failed to initialize SDL: %s", SDL_GetError());
    }
    SDL_version ver; SDL_GetVersion(&ver);
    fun_msg("SDL version %d.%d.%d", ver.major, ver.minor, ver.patch);

    // Create window
    const Uint32 WNDFLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    SDL_Window* wnd = SDL_CreateWindow("Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, WNDFLAGS);
    FUN_ASSERT(wnd && "Failed to create SDL window");

    // OpenGL 3.3 core context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GLContext gl = SDL_GL_CreateContext(wnd);
    FUN_ASSERT(gl && "Failed to create OpenGL context");
    SDL_GL_MakeCurrent(wnd, gl);

    // Load OpenGL functions
    if (glewInit() != GLEW_OK) {
        fun_err("Failed to load OpenGL functions");
    }

    // Enable debug output
    glDebugMessageCallback(on_gl_error, NULL);
    glEnable(GL_DEBUG_OUTPUT);

    // Upload cube mesh data to the GPU
    GLuint vao, vbo, ibo;
    // VAO - vertex attribute object - tracks vertex layout
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // VBO - vertex buffer object - stores vertex data
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES, GL_STATIC_DRAW);
    // Set vertex layout
    const GLsizei VERTEX_SIZE = sizeof(float) * (3 + 3 + 2);
    // vec3 position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(sizeof(float) * 0));
    glEnableVertexAttribArray(0);
    // vec3 normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    // vec2 texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(sizeof(float) * 6));
    glEnableVertexAttribArray(2);
    // IBO - index buffer object - stores index data
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES, GL_STATIC_DRAW);

    // Compile the OpenGL shader
    GLuint program = glCreateProgram();
    GLuint vs = compile_shader(CUBE_VS, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(CUBE_FS, GL_FRAGMENT_SHADER);
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        static char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fun_err("Failed to link OpenGL program: %s", log);
    }

    // Window loop
    bool running = true;
    do {
        // Poll events
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_QUIT: { running = false; } break;
            };
        }

        // Set OpenGL state
        glUseProgram(program);      // Use our shader for rendering
        glEnable(GL_DEPTH_TEST);    // Enable z-buffer testing

        // Update the viewport
        int vp_w, vp_h;
        SDL_GL_GetDrawableSize(wnd, &vp_w, &vp_h);
        glViewport(0, 0, vp_w, vp_h);

        // Update projection matrix
        float proj[4*4];
        fun_mat4_perspective(proj, FUN_DEG2RAD(90.0f/2.0f), (float)vp_w/(float)vp_h, 0.1f, 10.0f);

        // Update model matrix
        float model[4*4];
        {
            const float t = SDL_GetTicks() / 1e3f;
            // rotate around x
            float xrot[4*4]; fun_mat4_rotate(xrot, t, 1.0f, 0.0f, 0.0f);
            // rotate around z
            float zrot[4*4]; fun_mat4_rotate(zrot, t, 0.0f, 0.0f, 1.0f);
            // combine
            fun_mat4_mul(model, xrot, zrot);
        }
        model[3*4+2] = -1.5f; // move along -z
        
        fun_msg("Projection matrix:");
        for (int y = 0; y < 4; ++y) {
            printf("    ");
            for (int x = 0; x < 4; ++x) {
                printf("%.2f ", proj[y*4+x]);
            }
            printf("\n");
        };
        fun_msg("Model matrix:");
        for (int y = 0; y < 4; ++y) {
            printf("    ");
            for (int x = 0; x < 4; ++x) {
                printf("%.2f ", model[y*4+x]);
            }
            printf("\n");
        };

        glUniformMatrix4fv(glGetUniformLocation(program, "u_proj"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_model"), 1, GL_FALSE, model);

        // Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the cube
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, NULL);

        // Update the screen
        SDL_GL_SwapWindow(wnd);
    } while (running);

    SDL_DestroyWindow(wnd);
    SDL_Quit();

    return 0;
}
