#version 330 core

in vec2 v_t;

out vec4 color;

void main() {
    // https://madebyevan.com/shaders/grid/
    vec2 grid = abs(fract(v_t * 8 - 0.5f) - 0.5f) / fwidth(v_t) / 8;
    float fac = 1.0 - min(min(grid.x, grid.y), 1.0);
    fac = pow(fac, 1.0 / 2.2);

    color = vec4(mix(vec3(0.2f, 0.2f, 0.6f), vec3(1.0f, 1.0f, 1.0f), fac), 1.0f);
}