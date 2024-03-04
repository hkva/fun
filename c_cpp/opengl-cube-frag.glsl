#version 330 core

in vec2 v_t;

out vec4 color;

uniform float u_grid_scale;
uniform vec4  u_color;

void main() {
    // https://madebyevan.com/shaders/grid/
    vec2 grid = abs(fract(v_t * u_grid_scale - 0.5f) - 0.5f) / fwidth(v_t) / u_grid_scale;
    float fac = 1.0 - min(min(grid.x, grid.y), 1.0);
    fac = pow(fac, 1.0 / 2.2);

    color = mix(u_color, vec4(1.0f, 1.0f, 1.0f, 1.0f), fac);
}