// SPDX-License-Identifier: MIT

#define DEMO_NAME "Cube"
#include "opengl.hh"

const char* VS = R"""(
#version 330 core

layout (location = 0) in vec3 p;
layout (location = 1) in vec2 t;

uniform mat4 u_model;
uniform mat4 u_proj;

out vec2 v_t;

void main() {
    v_t = t;
    gl_Position = vec4(p, 1.0f) * u_model * u_proj;
}
)""";

// https://madebyevan.com/shaders/grid/
const char* FS = R"""(
#version 330 core

in vec2 v_t;

out vec4 color;

uniform float u_grid_scale;
uniform vec4  u_color;

void main() {
    vec2 grid = abs(fract(v_t * u_grid_scale - 0.5f) - 0.5f) / fwidth(v_t) / u_grid_scale;
    float fac = 1.0 - min(min(grid.x, grid.y), 1.0);
    fac = pow(fac, 1.0 / 2.2);
    color = mix(u_color, vec4(1.0f, 1.0f, 1.0f, 1.0f), fac);
}
)""";

typedef struct Vertex {
    Vec3 p;
    Vec2 t;
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
    Vec3 pos;
    Vec3 ang;
    f32 fov;
    f32 near;
    f32 far;
    f32 aspect;
} Camera;

typedef struct Model {
    Mesh mesh;
    Mat4 transform;
    f32 color[3];
    f32 grid_scale;
    GLuint prog;
    usize num_indices;
} Model;

GLuint prog;
Model cube = { };
Model plane = { };

void demo_init(const App* app) {
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Enable multisampling
    glEnable(GL_MULTISAMPLE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Compile shaders
    prog = compile_gl_program(VS, FS);

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

    const u32 cube_indices[] = {
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

    cube.color[0] = 0.3f; cube.color[1] = 0.3f; cube.color[2] = 0.6f;
    cube.grid_scale = 8.0f;
    cube.prog = prog;
    cube.num_indices = arrlen(cube_indices);

    // Vertex array object - stores array bindings
    glGenVertexArrays(1, &cube.mesh.vao);
    glBindVertexArray(cube.mesh.vao);

    // Vertex buffer object - stores vertex data
    glGenBuffers(1, &cube.mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube.mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);

    // Index buffer object - stores index data
    glGenBuffers(1, &cube.mesh.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    // Set vertex layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, t));
    glEnableVertexAttribArray(1);

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

    const u32 plane_indices[] = {
         0,  1,  2,
         0,  2,  3,
    };

    plane.color[0] = 0.4f; plane.color[1] = 0.4f; plane.color[2] = 0.4f;
    plane.grid_scale = 8.0f;
    plane.prog = prog;
    plane.num_indices = arrlen(plane_indices);

    glGenVertexArrays(1, &plane.mesh.vao);
    glBindVertexArray(plane.mesh.vao);

    glGenBuffers(1, &plane.mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, plane.mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_verts), plane_verts, GL_STATIC_DRAW);

    glGenBuffers(1, &plane.mesh.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane.mesh.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_indices), plane_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, t));
    glEnableVertexAttribArray(1);
}

static void draw_scene(const Model* models, usize num_models, Camera cam) {
    Mat4 cam_proj = Mat4::perspective(cam.near, cam.far, cam.aspect, cam.fov);

    Mat4 cam_transform = Mat4::translate(cam.pos.invert());

    for (usize i = 0; i < num_models; ++i) {
        const Model* m = &models[i];

        Mat4 model_transform = m->transform * cam_transform;

        glBindVertexArray(m->mesh.vao);

        glUseProgram(m->prog);

        glUniformMatrix4fv(glGetUniformLocation(m->prog, "u_model"), 1, GL_FALSE, model_transform.base());
        glUniformMatrix4fv(glGetUniformLocation(m->prog, "u_proj"), 1, GL_FALSE, cam_proj.base());

        glUniform1f(glGetUniformLocation(m->prog, "u_grid_scale"), m->grid_scale);
        glUniform4f(glGetUniformLocation(m->prog, "u_color"), m->color[0], m->color[1], m->color[2], 1.0f);

        glDrawElements(GL_TRIANGLES, m->num_indices, GL_UNSIGNED_INT, NULL);
    }
}

void demo_frame(const App* app) {
    // r: <t * 30.0f, t * 30.0f, 0.0f>
    cube.transform = Mat4::rotate_x(app->t * 30.0f) * Mat4::rotate_y(app->t * 30.0f);

    // p: <0.0f, -1.0f, 0.0>
    // r: <90.0f, 0.0f, 0.0f>
    // s: <4.0f, 4.0f, 4.0f>
    plane.transform =
        Mat4::rotate_x(90.0f) *
        (Mat4::scale(Vec3::broadcast(4.0f)) * Mat4::translate(Vec3(0.0f, -1.0f, 0.0f)));

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->vp.x, app->vp.y);

    Model scene[] = { cube, plane };

    //
    // Draw scene
    //

    Camera cam = { };
    cam.pos = Vec3(0.0f, 0.0f, 1.5f);
    cam.type = CAM_PERSPECTIVE;
    cam.fov = 90.0f;
    cam.near = 0.1f;
    cam.far = 20.0f;
    cam.aspect = app->vp.x / app->vp.y;

    draw_scene(scene, arrlen(scene), cam);    
}

void demo_ui(const App* app) {
}
