#version 330 core

in vec2 v_t;

out vec4 color;

uniform sampler2D u_atlas;

void main() {
    color = vec4(0.5f, 0.9f, 0.9f, texture(u_atlas, v_t).r);
    // color = vec4(1.0f);
}