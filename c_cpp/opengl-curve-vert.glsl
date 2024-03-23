#version 330 core

layout (location = 0) in vec2 p;
layout (location = 1) in vec4 c;

uniform mat4 u_proj;

out vec4 v_c;

void main() {
    v_c = c;
    gl_Position = vec4(vec3(p, -1.0f), 1.0f) * u_proj;
}