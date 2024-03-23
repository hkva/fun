// SPDX-License-Identifier: MIT

#include "SDL_keycode.h"
#include "SDL_video.h"
#include "hk.hh"
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "opengl.hh"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include "stb_truetype.h"

#include "nanosvg.h"

//
// Utility functions
//

struct AppInfo {
	Vec2 vp;
};

// Shitty hack thing to automatically register demos
class Demo {
public:
	const char* name;
	void (*init)();
	void (*frame)(const AppInfo*);
	void (*ui)();
public:
	Demo(const char* name, void(*init)(), void(*frame)(const AppInfo*), void(*ui)())
		: name(name), init(init), frame(frame), ui(ui) {
		get_demos().push_back(this);
	}
public:
	static std::vector<Demo*>& get_demos() {
		static std::vector<Demo*> demos = { };
		return demos;
	}
};
#define REGISTER_DEMO(...) static Demo g_demo_ ## __LINE__ (__VA_ARGS__)

//
// Demo: Cube
//

namespace cube {

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

static void init() {
    // Enable depth testing
    GLCHECK(glEnable(GL_DEPTH_TEST));

    // Enable multisampling
    GLCHECK(glEnable(GL_MULTISAMPLE));

    GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    // Compile shaders
    prog = compile_gl_program("opengl-cube-vert.glsl", "opengl-cube-frag.glsl");

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

    const u32 plane_indices[] = {
         0,  1,  2,
         0,  2,  3,
    };

    plane.color[0] = 0.4f; plane.color[1] = 0.4f; plane.color[2] = 0.4f;
    plane.grid_scale = 8.0f;
    plane.prog = prog;
    plane.num_indices = arrlen(plane_indices);

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
}

static void draw_scene(const Model* models, usize num_models, Camera cam) {
    Mat4 cam_proj = Mat4::perspective(cam.near, cam.far, cam.aspect, cam.fov);

    Mat4 cam_transform = Mat4::translate(cam.pos.invert());

    for (usize i = 0; i < num_models; ++i) {
        const Model* m = &models[i];

        Mat4 model_transform = m->transform * cam_transform;

        GLCHECK(glBindVertexArray(m->mesh.vao));

        GLCHECK(glUseProgram(m->prog));

        GLCHECK(glUniformMatrix4fv(glGetUniformLocation(m->prog, "u_model"), 1, GL_FALSE, model_transform.base()));
        GLCHECK(glUniformMatrix4fv(glGetUniformLocation(m->prog, "u_proj"), 1, GL_FALSE, cam_proj.base()));

        GLCHECK(glUniform1f(glGetUniformLocation(m->prog, "u_grid_scale"), m->grid_scale));
        GLCHECK(glUniform4f(glGetUniformLocation(m->prog, "u_color"), m->color[0], m->color[1], m->color[2], 1.0f));

        GLCHECK(glDrawElements(GL_TRIANGLES, m->num_indices, GL_UNSIGNED_INT, NULL));
    }
}

static void frame(const AppInfo* app) {
	const f32 t = (f32)SDL_GetTicks() / 1e3f;

	// r: <t * 30.0f, t * 30.0f, 0.0f>
	cube.transform = Mat4::rotate_x(t * 30.0f) * Mat4::rotate_y(t * 30.0f);

	// p: <0.0f, -1.0f, 0.0>
	// r: <90.0f, 0.0f, 0.0f>
	// s: <4.0f, 4.0f, 4.0f>
	plane.transform =
		Mat4::rotate_x(90.0f) *
		(Mat4::scale(Vec3::broadcast(4.0f)) * Mat4::translate(Vec3(0.0f, -1.0f, 0.0f)));

	GLCHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
	GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	GLCHECK(glViewport(0, 0, app->vp.x, app->vp.y));

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

static void ui() {

}

REGISTER_DEMO("Cube", init, frame, ui);

}

//
// Demo: Curves
//

namespace lines {

struct Vertex {
    Vec2 p;
    Vec4 c;
};

static GLuint vao;
static GLuint vbo;

static GLuint prog;

static f32 sine_step = 3.0f;
static f32 bezier_samples = 5.0f;

static u32 draws = 0;

static struct {
    NSVGimage* acid;
    NSVGimage* mozilla;
    NSVGimage* text;
} images = { };

static void init() {
    GLCHECK(glDisable(GL_DEPTH_TEST));
    GLCHECK(glEnable(GL_MULTISAMPLE));
    GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    prog = compile_gl_program("opengl-curve-vert.glsl", "opengl-curve-frag.glsl");

    GLCHECK(glGenVertexArrays(1, &vao));
    GLCHECK(glBindVertexArray(vao));

    GLCHECK(glGenBuffers(1, &vbo));
    GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 2, nullptr, GL_STREAM_DRAW));

    GLCHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p)));
    GLCHECK(glEnableVertexAttribArray(0));
    GLCHECK(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, c)));
    GLCHECK(glEnableVertexAttribArray(1));

    static struct {
        NSVGimage** image;
        const char* path;
    } image_sources[] = {
        { &images.acid, "data/acid.svg" },
        { &images.mozilla, "data/mozilla.svg" },
        { &images.text, "data/text.svg" },
    };
    for (usize i = 0; i < arrlen(image_sources); ++i) {
        if (!(*image_sources[i].image = nsvgParseFromFile(image_sources[i].path, "px", 96.0f))) {
            dbgerr("Failed to load SVG image %s", image_sources[i].path);
        }
        dbglog("Parsed %s", image_sources[i].path);
    }
}

static void draw_line(Vec2 p1, Vec2 p2, Vec4 color = Vec4(1.0f, 1.0f, 1.0f)) {
    // @@ batching
    Vertex line[2] = {
        { p1, color },
        { p2, color },
    };
    GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line));
    GLCHECK(glDrawArrays(GL_LINES, 0, 2));
    ++draws;
}

static Vec2 sample_bezier(const Vec2 p[4], float t) {
    auto pow2 = [](float f) { return f * f; };
    auto pow3 = [](float f) { return f * f * f; };

    return
        p[0].scale(pow3(1.0f - t)) +
        p[1].scale(3.0f * pow2(1.0f - t) * t) +
        p[2].scale(3.0f * (1.0f - t) * pow2(t)) +
        p[3].scale(pow3(t));
}

static void draw_bezier(const Vec2 p[4], Vec4 color) {
    const f32 step = (1.0f / bezier_samples);
    for (f32 i = 0; i < 1.0f; i += step) {
        const Vec2 p1 = sample_bezier(p, i);
        const Vec2 p2 = sample_bezier(p, i + min(step, 1.0f - i));
        draw_line(p1, p2, color);
    }
}

static void draw_svg(NSVGimage* image, Vec2 pos, f32 scale = 1.0f, Vec4 color = Vec4(0.0f, 1.0f, 0.0f, 1.0f)) {
    for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path; path = path->next) {
            for (i32 i = 0; i < path->npts - 1; i += 3) {
                f32* p = &path->pts[i*2];
                const Vec2 pts[] = {
                    pos + Vec2(p[0], p[1]).scale(scale),
                    pos + Vec2(p[2], p[3]).scale(scale),
                    pos + Vec2(p[4], p[5]).scale(scale),
                    pos + Vec2(p[6], p[7]).scale(scale)
                };

                // draw_line(pos + p1.scale(scale), pos + p4.scale(scale), Vec4(1.0f, 1.0f, 0.0f));
                draw_bezier(pts, color);
            }
        }
    }
}

static void frame(const AppInfo* app) {
    draws = 0;

    glLineWidth(1.0f);

    GLCHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
    GLCHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GLCHECK(glViewport(0, 0, app->vp.x, app->vp.y));

    const Mat4 proj = Mat4::orthographic(0.1f, 2.0f, 0.0f, app->vp.x, 0.0f, app->vp.y);

    GLCHECK(glUseProgram(prog));
    GLCHECK(glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"), 1, GL_FALSE, proj.base()))

    draw_line(Vec2(100, 100), Vec2(200, 200));

    // sine
    for (f32 x = 400.0f; x < 1000.0f; x += sine_step) {
        Vec2 p1 = Vec2(x, 200.0f + sinf((x / 20.0f)) * 50.0f);
        Vec2 p2 = Vec2(x + sine_step, 200.0f + sinf((x + sine_step) / 20.0f) * 50.0f);
        draw_line(p1, p2);
    }

    draw_svg(images.acid, Vec2(500.0f, 400.0f), 2.0f);
    draw_svg(images.mozilla, Vec2(800.0f, 400.0f), 2.0f);
    draw_svg(images.text, Vec2(15.0f, 300.0f), 2.0f);
}

static void ui() {
    // static bool draw_lines = false;
    // if (ImGui::Checkbox("Wireframe", &draw_lines)) {
    //     dbglog("clicked");
    //     glPolygonMode(GL_FRONT_AND_BACK, (draw_lines) ? GL_LINE : GL_FILL);
    // }

    ImGui::SliderFloat("Sine wave step", &sine_step, 1.0f, 50.0f);
    ImGui::SliderFloat("Bezier samples", &bezier_samples, 1.0f, 10.0f);
    ImGui::Text("Segments: %u", draws);
}

REGISTER_DEMO("Lines", init, frame, ui);

}

//
// Demo: Text
//

namespace text {

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
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, r.vbo));
        GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * r.head * 4, r.batch));
        GLCHECK(glDrawElements(GL_TRIANGLES, r.head * 6, GL_UNSIGNED_INT, nullptr));
        ++num_draws;
        r.head = 0;
    }

    if (leftover != nullptr) {
        draw_text(pos, leftover, flush);
    }
}

static void init() {
	old_vp = Vec2(-1.0f, -1.0f);

    // Disable depth testing
    GLCHECK(glDisable(GL_DEPTH_TEST));

    // Enable multisampling
    GLCHECK(glEnable(GL_MULTISAMPLE));

    GLCHECK(glEnable(GL_MULTISAMPLE));
    GLCHECK(glEnable(GL_BLEND));
    GLCHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

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

    prog = compile_gl_program("opengl-text-vert.glsl", "opengl-text-frag.glsl");
    post = compile_gl_program("opengl-text-vert.glsl", "opengl-text-post-frag.glsl");

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
    GLCHECK(glGenTextures(1, &tex));
    GLCHECK(glBindTexture(GL_TEXTURE_2D, tex));
    GLCHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]));
    GLCHECK(glGenerateMipmap(GL_TEXTURE_2D));

    // Frame buffer
    GLCHECK(glGenFramebuffers(1, &fbo));

    // Color attachment
    GLCHECK(glGenTextures(1, &fbo_tex));
}

static void frame(const AppInfo* app) {
    num_chars = 0; num_draws = 0;

	GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

	// Update framebuffer texture
	if (app->vp.x != old_vp.x || app->vp.y != old_vp.y) {
		old_vp = app->vp;

		GLCHECK(glBindTexture(GL_TEXTURE_2D, fbo_tex));
		GLCHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, app->vp.x, app->vp.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));
		GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0));
	}

	GLCHECK(glViewport(0, 0, app->vp.x, app->vp.y));
	GLCHECK(glClearColor(0.1f, 0.1f, 0.1f, 1.0f));
	GLCHECK(glClear(GL_COLOR_BUFFER_BIT));

	const Mat4 proj = Mat4::orthographic(0.1f, 2.0f, 0.0f, app->vp.x, 0.0f, app->vp.y);

	GLCHECK(glUseProgram(prog));
	GLCHECK(glUniform1i(glGetUniformLocation(prog, "u_atlas"), 0));
	GLCHECK(glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"), 1, GL_FALSE, proj.base()))

	// Sample font texture
	GLCHECK(glBindTexture(GL_TEXTURE_2D, tex));

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
	GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad));
	GLCHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
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
	GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GLCHECK(glUseProgram(post));
	GLCHECK(glUniform1i(glGetUniformLocation(post, "u_atlas"), 0));
	GLCHECK(glUniformMatrix4fv(glGetUniformLocation(post, "u_proj"), 1, GL_FALSE, proj.base()))
	GLCHECK(glUniform2f(glGetUniformLocation(post, "u_vp"), app->vp.x, app->vp.y));
	GLCHECK(glBindTexture(GL_TEXTURE_2D, fbo_tex));

	GLCHECK(glClear(GL_COLOR_BUFFER_BIT));

	const Vec2 fb_p0 = Vec2(0.0f, 0.0f);
	const Vec2 fb_p1 = app->vp;
	Vertex fb_quad[] = {
		{ fb_p0,                    Vec2(0.0f, 1.0f) },
		{ Vec2(fb_p1.x, fb_p0.y),   Vec2(1.0f, 1.0f) },
		{ fb_p1,                    Vec2(1.0f, 0.0f) },
		{ Vec2(fb_p0.x, fb_p1.y),   Vec2(0.0f, 0.0f) },
	};
	GLCHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fb_quad), fb_quad));
	GLCHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
}

static void ui() {
    ImGui::Text("# chars: %u", num_chars);
    ImGui::Text("# draws: %u (%u / %u)", num_draws, num_chars, (u32)BATCH_SIZE);
    float dpi = 0.0f; SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
    ImGui::Text("DPI: %f", dpi);
    ImGui::Text("DPI / 96.0f: %f", dpi / 96.0f);
    ImGui::Text("DPI / 72.0f: %f", dpi / 72.0f);
}

REGISTER_DEMO("Text", init, frame, ui);

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
    SDL_Window* wnd = SDL_CreateWindow("OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, wndflags);
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplSDL2_InitForOpenGL(wnd, gl);
    ImGui_ImplOpenGL3_Init();

    dbglog("GL_VERSION:  %s", glGetString(GL_VERSION));   GLCHECK();
    dbglog("GL_VENDOR:   %s", glGetString(GL_VENDOR));    GLCHECK();
    dbglog("GL_RENDERER: %s", glGetString(GL_RENDERER));  GLCHECK();

    static usize selected_demo = 0;
    static usize last_selected_demo = (usize)-1;

    if (argc >= 2) {
        for (usize i = 0; i < Demo::get_demos().size(); ++i) {
            if (str::ieq(Demo::get_demos()[i]->name, argv[1])) {
                selected_demo = i;
            }
        }
    }

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
            	case SDLK_r: {
            		dbglog("Reloading demo \"%s\"", Demo::get_demos()[selected_demo]->name);
            		Demo::get_demos()[selected_demo]->init();
            	} break;
                }
            } break;
            }
        }

        Vec2 vp = Vec2();
        {
        	i32 vp_w; i32 vp_h;
        	SDL_GL_GetDrawableSize(wnd, &vp_w, &vp_h);
        	vp = Vec2((f32)vp_w, (f32)vp_h);
        }

        if (selected_demo != last_selected_demo) {
			Demo::get_demos()[selected_demo]->init();
        	last_selected_demo = selected_demo;

        	char title[64]; snprintf(title, sizeof(title), "OpenGL: %s", Demo::get_demos()[selected_demo]->name);
        	SDL_SetWindowTitle(wnd, title);
        }

        AppInfo app = { };
        app.vp = vp;
        Demo::get_demos()[selected_demo]->frame(&app);

        // Render UI
        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(15.0f, 15.0f));
        static bool window_hovered = false;
        if (window_hovered) {
        	ImGui::SetNextWindowSize(ImVec2(300.0f, vp.y - 30.0f));
        } else {
        	ImGui::SetNextWindowSize(ImVec2(300.0f, 37.0f));
        }
        if (ImGui::Begin("OpenGL", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        	if (ImGui::BeginCombo("##demo", Demo::get_demos()[selected_demo]->name)) {
        		for (usize i = 0; i < Demo::get_demos().size(); ++i) {
        			if (ImGui::Selectable(Demo::get_demos()[i]->name, i == selected_demo)) {
        				selected_demo = i;
        			}
        			if (i == selected_demo) {
        				ImGui::SetItemDefaultFocus();
        			}
        		}
        		ImGui::EndCombo();
        	}

        	window_hovered = ImGui::IsWindowHovered() || ImGui::IsAnyItemActive();
        	if (window_hovered) {
        		Demo::get_demos()[selected_demo]->ui();
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
