#include "fun.h"

#include "cgltf.h"
#include <GL/glew.h>
#include <SDL.h>

// Vertex shader
static const char* GLTF_VS =
    "#version 330 core                                  \n"
    "                                                   \n"
    "layout (location = 0) in vec3 p;                   \n"
    "layout (location = 1) in vec3 n;                   \n"
    "layout (location = 2) in vec2 t;                   \n"
    "                                                   \n"
    "uniform mat4 u_view;                               \n"
    "uniform mat4 u_model;                              \n"
    "                                                   \n"
    "out vec2 v_t;                                      \n"
    "                                                   \n"
    "void main() {                                      \n"
    "   v_t = t;                                        \n"
    "   gl_Position = u_view * u_model * vec4(p, 1.0f); \n"
    "}                                                  \n";

// Fragment shader
static const char* GLTF_FS =
    "#version 330 core                  \n"
    "                                   \n"
    "in vec2 v_t;                       \n"
    "                                   \n"
    "uniform sampler2D u_tex;           \n"
    "                                   \n"
    "out vec4 color;                    \n"
    "                                   \n"
    "void main() {                      \n"
    "   color = texture(u_tex, v_t);    \n"
    "}                                  \n";

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

// Vertex type
typedef struct Vertex {
    float p[3];
    float n[3];
    float t[2];
} Vertex;

// Texture data
typedef struct Texture {
    cgltf_texture*  gtex;
    GLuint          tex;
} Texture;

// Mesh data
typedef struct Mesh {
    cgltf_mesh* gmesh;
    GLuint      vao;
    GLuint      vbo;
    GLuint      ibo;
    GLuint      tex;
    unsigned    indices;
} Mesh;

// Scene graph
typedef struct Node Node;
struct Node {
    Mesh*   mesh;
    float   transform[16];

    Node*   children;
    size_t  children_count;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fun_err("Usage: %s <glTF path>\n", argv[0]);
    }
    const char* gltf_path = argv[1];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fun_err("Failed to initialize SDL: %s", SDL_GetError());
    }
    SDL_version ver; SDL_GetVersion(&ver);
    fun_msg("SDL version %d.%d.%d", ver.major, ver.minor, ver.patch);

    // Create window
    const Uint32 WNDFLAGS = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    SDL_Window* wnd = SDL_CreateWindow("glTF Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, WNDFLAGS);
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

    // Compile the OpenGL shader
    GLuint program = glCreateProgram();
    GLuint vs = compile_shader(GLTF_VS, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(GLTF_FS, GL_FRAGMENT_SHADER);
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status) {
        static char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fun_err("Failed to link OpenGL program: %s", log);
    }

    // Parse the model
    cgltf_options opts = { 0 };
    cgltf_data* gltf = NULL;
    if (cgltf_parse_file(&opts, gltf_path, &gltf) != cgltf_result_success) {
        fun_err("Failed to parse glTF file %s", gltf_path);
    }
    if (cgltf_load_buffers(&opts, gltf, gltf_path) != cgltf_result_success) {
        fun_err("Failed to load glTF buffers");
    }

    // Textures URIs are relative to the glTF model
    fun_chdir(fun_basename(gltf_path));

    // First upload all textures
    Texture* textures = malloc(sizeof(Texture) * gltf->textures_count); FUN_ASSERT(textures);
    for (size_t i = 0; i < gltf->textures_count; ++i) {
        cgltf_texture* gtex = &gltf->textures[i];
        // Textures can be embedded or point to a file
        fun_image_t img = { 0 };
        if (gtex->image->buffer_view != NULL) {
            if (!fun_image_from_memory(&img, cgltf_buffer_view_data(gtex->image->buffer_view), gtex->image->buffer_view->size)) {
                fun_err("Failed to parse texture #%zu from memory", i+1);
            }
        } else {
            if (!fun_image_from_file(&img, gtex->image->uri)) {
                fun_err("Failed to parse texture %s", gtex->image->uri);
            }
        }

        textures[i].gtex = gtex;
        
        glGenTextures(1, &textures[i].tex);
        glBindTexture(GL_TEXTURE_2D, textures[i].tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        fun_msg("[%zu/%zu] Uploaded texture %s", i+1, gltf->textures_count, (gtex->name) ? gtex->name : "<unnamed>");
    }

    // Next upload meshes
    Mesh* meshes = malloc(sizeof(Mesh) * gltf->meshes_count); FUN_ASSERT(meshes);
    for (size_t i = 0; i < gltf->meshes_count; ++i) {
        cgltf_mesh* gmesh = &gltf->meshes[i];
        Mesh* mesh = &meshes[i];
        // Only allow one primitive per mesh
        FUN_ASSERT(gmesh->primitives_count >= 1);
        if (gmesh->primitives_count > 1) {
            fun_msg("Warning: Mesh #%zu has more than one primitive", i);
        }
        cgltf_primitive* gprim = &gmesh->primitives[0];
        // Expect 1P1N1T vertex layout, skip everything else
        cgltf_attribute* gattr_p = NULL;
        cgltf_attribute* gattr_n = NULL;
        cgltf_attribute* gattr_t = NULL;
        for (size_t attr_idx = 0; attr_idx < gprim->attributes_count; ++attr_idx) {
            cgltf_attribute* gattr = &gprim->attributes[attr_idx];
            switch (gattr->type) {
                case cgltf_attribute_type_position: {
                    gattr_p = gattr;
                } break;
                case cgltf_attribute_type_normal: {
                    gattr_n = gattr;
                } break;
                case cgltf_attribute_type_texcoord: {
                    gattr_t = gattr;
                } break;
                default: break;
            };
        }
        FUN_ASSERT(gattr_p != NULL && gattr_p->data != NULL);
        FUN_ASSERT(gattr_n != NULL && gattr_n->data != NULL);
        FUN_ASSERT(gattr_t != NULL && gattr_t->data != NULL);
        FUN_ASSERT(gattr_p->data->count == gattr_n->data->count);
        FUN_ASSERT(gattr_p->data->count == gattr_t->data->count);
        // VAO
        glGenVertexArrays(1, &mesh->vao);
        glBindVertexArray(mesh->vao);
        // VBO
        glGenBuffers(1, &mesh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * gattr_p->data->count, NULL, GL_STATIC_DRAW);
        // Unpack vertex data
        Vertex* verts = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        for (size_t j = 0; j < gattr_p->data->count; ++j) {
            Vertex* v = &verts[j];
            // p
            if (!cgltf_accessor_read_float(gattr_p->data, j, v->p, 3)) {
                fun_err("Failed to read position data at index %lu", j);
            }
            // n
            if (!cgltf_accessor_read_float(gattr_n->data, j, v->n, 3)) {
                fun_err("Failed to read normal data at index %lu", j);
            }
            // t
            if (!cgltf_accessor_read_float(gattr_t->data, j, v->t, 2)) {
                fun_err("Failed to read texcoord data at index %lu", j);
            }
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        // Set layout
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, t));
        glEnableVertexAttribArray(2);
        // IBO
        glGenBuffers(1, &mesh->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * gprim->indices->count, NULL, GL_STATIC_DRAW);
        // Unpack index data
        unsigned int* indices = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        for (size_t j = 0; j < gprim->indices->count; ++j) {
            indices[j] = cgltf_accessor_read_index(gprim->indices, j);
        }
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        mesh->indices = gprim->indices->count;
        // Map texture
        FUN_ASSERT(gprim->material->has_pbr_metallic_roughness);
        FUN_ASSERT(gprim->material->pbr_metallic_roughness.base_color_texture.texture);
        size_t tex_idx = gprim->material->pbr_metallic_roughness.base_color_texture.texture - gltf->textures;
        mesh->tex = textures[tex_idx].tex;

        fun_msg("[%zu/%zu] Uploaded mesh %s", i+1, gltf->meshes_count, gmesh->name);
    }

    // Walk scene and create nodes
    Node* root = malloc(sizeof(Node)); FUN_ASSERT(root);
    FUN_ZERO(*root);
    root->children_count = gltf->scene->nodes_count;
    root->children = malloc(sizeof(Node) * root->children_count); FUN_ASSERT(root->children);
    for (size_t i = 0; i < gltf->scene->nodes_count; ++i) {
        FUN_ZERO(root->children[i]);
        // Assume flat scene structure for now
        cgltf_node* gnode = gltf->scene->nodes[i];
        Node* node = &root->children[i];
        node->mesh = &meshes[gnode->mesh - gltf->meshes];
        cgltf_node_transform_world(gnode, node->transform);
    }

    // Gamera state
    float cam_pos[3] = { 0 };
    float cam_vel[3] = { 0 };
    float cam_ang[2] = { 0 };
    float cam_fov = 75.0f;

    // Window loop
    float then = 0.0f;
    SDL_ShowWindow(wnd);
    bool running = true;
    do {
        // Poll events
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_QUIT: { running = false; } break;
                case SDL_KEYDOWN: {
                    if (evt.key.repeat > 0) { break; }
                    switch (evt.key.keysym.sym) {
                        case SDLK_w: { cam_vel[2] -= 1.0f; } break;
                        case SDLK_a: { cam_vel[0] -= 1.0f; } break;
                        case SDLK_s: { cam_vel[2] += 1.0f; } break;
                        case SDLK_d: { cam_vel[0] += 1.0f; } break;
                        case SDLK_SPACE: { cam_vel[1] += 1.0f; } break;
                        case SDLK_LSHIFT: { cam_vel[1] -= 1.0f; } break;

                        case SDLK_ESCAPE: {
                            if (SDL_GetRelativeMouseMode()) {
                                int x, y;
                                SDL_GetWindowSize(wnd, &x, &y);
                                SDL_WarpMouseInWindow(wnd, x/2, y/2);
                                SDL_SetRelativeMouseMode(false);
                            }
                        } break;
                    };
                } break;
                case SDL_KEYUP: {
                    if (evt.key.repeat > 0) { break; }
                    switch (evt.key.keysym.sym) {
                        case SDLK_w: { cam_vel[2] += 1.0f; } break;
                        case SDLK_a: { cam_vel[0] += 1.0f; } break;
                        case SDLK_s: { cam_vel[2] -= 1.0f; } break;
                        case SDLK_d: { cam_vel[0] -= 1.0f; } break;
                        case SDLK_SPACE: { cam_vel[1] -= 1.0f; } break;
                        case SDLK_LSHIFT: { cam_vel[1] += 1.0f; } break;
                    };
                } break;
                case SDL_MOUSEBUTTONDOWN: {
                    if (evt.button.button == SDL_BUTTON_LEFT) {
                        SDL_SetRelativeMouseMode(true);
                    }
                } break;
                case SDL_MOUSEMOTION: {
                    if (SDL_GetRelativeMouseMode()) {
                        cam_ang[0] += (float)evt.motion.xrel / 1000.0f;
                        cam_ang[1] += (float)evt.motion.yrel / 1000.0f;
                    }
                } break;
                case SDL_MOUSEWHEEL: {
                    cam_fov += evt.wheel.y;
                };
            };
        }

        // Delta time
        const float now = SDL_GetTicks() / 1e3f;
        const float dt = now - then;
        then = now;

        // Rotate velocity vector by yaw
        float t = cam_ang[0];
        float cam_vel_local[3];
        cam_vel_local[0] = cosf(t) * cam_vel[0] - sinf(t) * cam_vel[2];
        cam_vel_local[1] = cam_vel[1];
        cam_vel_local[2] = sinf(t) * cam_vel[0] + cosf(t) * cam_vel[2];

        // Update camera movement
        for (int i = 0; i < 3; ++i) {
            cam_pos[i] += cam_vel_local[i] * dt;
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
        fun_mat4_perspective(proj, FUN_DEG2RAD(cam_fov/2.0f), (float)vp_w/(float)vp_h, 0.01f, 10.0f);

        // Update camera matrix
        // translate
        float cam_trans[4*4]; fun_mat4_ident(cam_trans);
        cam_trans[3*4+0] = -cam_pos[0];
        cam_trans[3*4+1] = -cam_pos[1];
        cam_trans[3*4+2] = -cam_pos[2];
        // rotate
        float cam_rotx[4*4];
        fun_mat4_rotate(cam_rotx, cam_ang[0], 0.0f, 1.0f, 0.0f); // around y
        float cam_roty[4*4];
        fun_mat4_rotate(cam_roty, cam_ang[1], 1.0f, 0.0f, 0.0f); // around x
        float cam_rot[4*4];
        fun_mat4_mul(cam_rot, cam_rotx, cam_roty);
        // combine
        float cam[4*4];
        fun_mat4_mul(cam, cam_trans, cam_rot);

        // Update view matrix
        float view[4*4];
        fun_mat4_mul(view, cam, proj);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, view);

        // Clear
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the scene
        for (size_t i = 0; i < root->children_count; ++i) {
            Node* node = &root->children[i];
            glBindVertexArray(node->mesh->vao);
            glUniformMatrix4fv(glGetUniformLocation(program, "u_model"), 1, GL_FALSE, node->transform);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, node->mesh->tex);
            glDrawElements(GL_TRIANGLES, node->mesh->indices, GL_UNSIGNED_INT, NULL);
        }
        
        // Update the screen
        SDL_GL_SwapWindow(wnd);
    } while (running);

    SDL_DestroyWindow(wnd);
    SDL_Quit();

    return 0;
}
