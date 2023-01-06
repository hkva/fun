#version 460 core

layout (location = 0) in vec2 v_t;

layout (location = 0) out vec4 color;

void main() {
    int checker = 16;
    if ((int(v_t.x * checker) % 2 == 0) ^^ (int(v_t.y * checker) % 2 == 0)) {
        color = vec4(0.47f, 0.55f, 0.81f, 1.0f);
    } else {
        color = vec4(0.29f, 0.38f, 0.68f, 1.0f);
    }
}
