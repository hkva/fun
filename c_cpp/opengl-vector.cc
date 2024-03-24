// SPDX-License-Identifier: MIT

#include "imgui.h"
#define DEMO_NAME "Vector"
#include "opengl.hh"

#include "nanosvg.h"

const char* VS = R"""(
#version 330 core

layout (location = 0) in vec2 p;
layout (location = 1) in vec4 c;

uniform mat4 u_proj;

out vec4 v_c;

void main() {
    v_c = c;
    gl_Position = vec4(vec3(p, -1.0f), 1.0f) * u_proj;
}
)""";

const char* FS = R"""(
#version 330 core

in vec4 v_c;

out vec4 color;

void main() {
    color = v_c;
}
)""";

struct Vertex {
    Vec2 p;
    Vec4 c;
};

static GLuint vao;
static GLuint vbo;

static GLuint prog;

// static f32 sine_step = 3.0f;
static f32 bezier_samples = 5.0f;

static u32 lines = 0;
static u32 draws = 0;

static bool animated = true;

static struct {
    NSVGimage* acid;
    NSVGimage* mozilla;
    NSVGimage* text;
    NSVGimage* t4012;
} images = { };

static constexpr usize BATCH_COUNT = 1024;

static struct {
    Vertex batch[BATCH_COUNT * 2];
    usize head;
} batch = { };

void demo_init(const App* app) {
    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    prog = compile_gl_program(VS, FS);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 2 * BATCH_COUNT, nullptr, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, p));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, c));
    glEnableVertexAttribArray(1);

    static struct {
        NSVGimage** image;
        const char* path;
    } image_sources[] = {
        { &images.acid, "data/acid.svg" },
        { &images.mozilla, "data/mozilla.svg" },
        { &images.text, "data/text.svg" },
        { &images.t4012, "data/4012.svg" },
    };
    for (usize i = 0; i < arrlen(image_sources); ++i) {
        if (!(*image_sources[i].image = nsvgParseFromFile(image_sources[i].path, "px", 96.0f))) {
            dbgerr("Failed to load SVG image %s", image_sources[i].path);
        }
        dbglog("Parsed %s", image_sources[i].path);
    }
}

static void flush() {
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * batch.head * 2, batch.batch);
    glDrawArrays(GL_LINES, 0, batch.head * 2);
    ++draws;
    batch.head = 0;
}

static void draw_line(Vec2 p1, Vec2 p2, Vec4 color = Vec4(1.0f, 1.0f, 1.0f)) {
    if (batch.head + 2 >= BATCH_COUNT) {
        flush();
    }
    assert(batch.head + 2 < BATCH_COUNT);
    batch.batch[batch.head++] = { p1, color };
    batch.batch[batch.head++] = { p2, color };
    ++lines;
}

static Vec2 sample_bezier(const Vec2 p[4], float t) {
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

static void draw_svg(NSVGimage* image, Vec2 pos, Vec2 scale = Vec2(1.0f, 1.0f), usize slow = 1, Vec4 color = Vec4(0.2f, 1.0f, 0.2f, 1.0f)) {
    usize total_points = 0;
    for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path; path = path->next) {
            total_points += path->npts;
        }
    }

    usize points_per_second = 5;

    usize max_points = (SDL_GetTicks() / (points_per_second * slow)) % total_points;

    usize points = 0;
    for (NSVGshape* shape = image->shapes; shape; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path; path = path->next) {
            for (i32 i = 0; i < path->npts - 1; i += 3) {
                f32* p = &path->pts[i*2];
                const Vec2 pts[] = {
                    pos + Vec2(p[0], p[1]) * scale,
                    pos + Vec2(p[2], p[3]) * scale,
                    pos + Vec2(p[4], p[5]) * scale,
                    pos + Vec2(p[6], p[7]) * scale
                };

                if (animated && (points += 3) >= max_points) {
                    draw_line(pos, pts[3], Vec4(0.2f, 1.0f, 0.2f, 0.2f));
                    return;
                }

                draw_bezier(pts, ((max_points - points) / 3 > 5 || !animated) ? color : Vec4(0.7f, 1.0f, 0.7f, 1.0f));
            }
        }
    }
}

void demo_frame(const App* app) {
    draws = 0;
    lines = 0;

    glLineWidth(2.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->vp.x, app->vp.y);

    const Mat4 proj = Mat4::orthographic(0.1f, 2.0f, 0.0f, app->vp.x, 0.0f, app->vp.y);

    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_proj"), 1, GL_FALSE, proj.base());

    // draw_line(Vec2(100, 100), Vec2(200, 200));

    // // sine
    // for (f32 x = 400.0f; x < 1000.0f; x += sine_step) {
    //     Vec2 p1 = Vec2(x, 200.0f + sinf((x / 20.0f)) * 50.0f);
    //     Vec2 p2 = Vec2(x + sine_step, 200.0f + sinf((x + sine_step) / 20.0f) * 50.0f);
    //     draw_line(p1, p2);
    // }

    Vec2 scale = app->vp / Vec2(1280.0f, 720.0f);
    draw_svg(images.acid, Vec2(500.0f, 400.0f) * scale, scale * Vec2(2.0f, 2.0f));
    draw_svg(images.mozilla, Vec2(800.0f, 400.0f) * scale, scale * Vec2(2.0f, 2.0f));
    draw_svg(images.text, Vec2(15.0f, 200.0f) * scale, scale * Vec2(2.0f, 2.0f));
    draw_svg(images.t4012, Vec2(15.0f, 300.0f) * scale, scale * Vec2(2.0f, 2.0f), 2);

    flush();
}

void demo_ui(const App* app) {
    // static bool draw_lines = false;
    // if (ImGui::Checkbox("Wireframe", &draw_lines)) {
    //     dbglog("clicked");
    //     glPolygonMode(GL_FRONT_AND_BACK, (draw_lines) ? GL_LINE : GL_FILL);
    // }

    // ImGui::SliderFloat("Sine wave step", &sine_step, 1.0f, 50.0f);
    ImGui::SliderFloat("Bezier samples", &bezier_samples, 1.0f, 25.0f);
    ImGui::Checkbox("Animated", &animated);
    ImGui::Text("%u line segments", lines);
    ImGui::Text("%u draw calls", draws);
}
