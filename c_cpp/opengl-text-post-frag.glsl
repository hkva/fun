#version 330 core

in vec2 v_t;

out vec4 color;

uniform sampler2D u_atlas;
uniform vec2 u_vp;

void main() {
    vec2 cabbr_r = vec2(-1.0f, 0.0f);
    vec2 cabbr_g = vec2(0.0f, 0.0f);
    vec2 cabbr_b = vec2(1.0f, 0.0f);

    float r = texture(u_atlas, v_t + (cabbr_r / u_vp)).r;
    float g = texture(u_atlas, v_t + (cabbr_g / u_vp)).g;
    float b = texture(u_atlas, v_t + (cabbr_b / u_vp)).b; 

    color = vec4(r, g, b, 1.0f);
}