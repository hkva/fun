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