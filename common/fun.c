#include "fun.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Platform-specific stuff
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void fun_err(const char* fmt, ...) {
    static char buf[512];
    va_list va; va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    fprintf(stderr, "ERROR: %s\n", buf); fflush(stderr);

    abort();
}

void fun_msg(const char* fmt, ...) {
    static char buf[512];
    va_list va; va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    fprintf(stdout, "INFO: %s\n", buf); fflush(stdout);
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Math
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void fun_mat4_ident(float* restrict m) {
    m[0*4+0]=1.0f; m[0*4+1]=0.0f; m[0*4+2]=0.0f; m[0*4+3]=0.0f;
    m[1*4+0]=0.0f; m[1*4+1]=1.0f; m[1*4+2]=0.0f; m[1*4+3]=0.0f;
    m[2*4+0]=0.0f; m[2*4+1]=0.0f; m[2*4+2]=1.0f; m[2*4+3]=0.0f;
    m[3*4+0]=0.0f; m[3*4+1]=0.0f; m[3*4+2]=0.0f; m[3*4+3]=1.0f;
}

void fun_mat4_mul(float* restrict dst, const float* restrict l, const float* restrict r) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            dst[i*4+j] =
                l[i*4+0] * r[0*4+j] +
                l[i*4+1] * r[1*4+j] +
                l[i*4+2] * r[2*4+j] +
                l[i*4+3] * r[3*4+j];
        }
    }
}

void fun_mat4_perspective(float* restrict m, float fovy_rad, float aspect, float z_near, float z_far) {
    memset(m, 0, sizeof(float) * 4 * 4);
    const float fov_inv = 1.0f / tanf(fovy_rad);
    m[0*4+0]= fov_inv / aspect;
    m[1*4+1]= fov_inv;
    m[2*4+2]= -(z_far + z_near) / (z_far - z_near);
    m[2*4+3]= -1.0f;
    m[3*4+2]= -2.0f * (z_far * z_near) / (z_far - z_near);
}

void fun_mat4_rotate(float* restrict m, float rad, float x, float y, float z) {
    // My math is not strong enough to understand this in 3D
    // https://github.com/ands/lightmapper/blob/4fd3bf4e2c07263f85d5d875ebdef061bc512dd4/example/example.c#L480-L488
    const float s = sinf(rad); const float c = cosf(rad); const float c2 = 1.0f - c;
    m[0*4+0]=x*x*c2+c;      m[0*4+1]=x*y*c2+z*s;    m[0*4+2]=x*z*c2-y*s;    m[0*4+3]=0.0f;
    m[1*4+0]=x*y*c2-z*s;    m[1*4+1]=y*y*c2+c;      m[1*4+2]=y*z*c2+x*s;    m[1*4+3]=0.0f;
    m[2*4+0]=x*z*c2+y*s;    m[2*4+1]=y*z*c2-x*s;    m[2*4+2]=z*z*c2+c;      m[2*4+3]=0.0f;
    m[3*4+0]=0.0f;          m[3*4+1]=0.0f;          m[3*4+2]=0.0f;          m[3*4+3]=1.0f;
}

