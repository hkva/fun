#version 460 core

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in vec2 t;

layout (std140, binding = 0) uniform u_transforms {
    mat4 u_proj;
    mat4 u_model;
};

layout (location = 0) out vec2 v_t;

void main() {
    v_t = t;
    gl_Position = u_proj * u_model * vec4(p, 1.0f);
}
