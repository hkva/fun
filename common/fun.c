#include "fun.h"

#include "spng.h"

#ifdef FUN_LINUX
    #include <unistd.h>
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Platform-specific stuff
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

static char* _fun_strdup(const char* str) {
    char* r = malloc(strlen(str)+1); FUN_ASSERT(r);
    strcpy(r, str);
    return r;
}

char* fun_basename(const char* path) {
    char* result = _fun_strdup(path); FUN_ASSERT(result);
    char* last_delim = NULL;
    for (size_t i = 0; result[i] != '\0'; ++i) {
        if ((result[i] == '/' || result[i] == '\\') && result[i+1]!= '\0') {
            last_delim = &result[i];
        }
    }
    if (last_delim) {
        *last_delim = '\0';
    }
    return result;
}

void fun_chdir(const char* path) {
    fun_msg("fun_chdir(): Changing to %s", path);
#ifdef FUN_LINUX
    if (chdir(path) == -1) {
        fun_msg("chdir() failed");
    }
#endif
}

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
    m[0*4+0] = fov_inv / aspect;
    m[1*4+1] = fov_inv;
    m[2*4+2] = -(z_far + z_near) / (z_far - z_near);
    m[2*4+3] = -1.0f;
    m[3*4+2] = -2.0f * (z_far * z_near) / (z_far - z_near);
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

void fun_mat4_orthographic(float* restrict m, float l, float r, float t, float b, float n, float f) {
    memset(m, 0, sizeof(float) * 4 * 4);
    m[0*4+0] = 2.0f / (r - l);
    m[3*4+0] = -(r + l) / (r - l);
    m[1*4+1] = 2.0f / (t - b);
    m[3*4+1] = -(t + b) / (t - b);
    m[2*4+2] = -2.0f / (f - n);
    m[3*4+2] = -(f + n) / (f - n);
    m[3*4+3] = 1.0f;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// IO
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

uint8_t* fun_load_binary_file(const char* path, size_t* length_out) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fun_msg("File not found");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (length_out) {
        *length_out = length;
    }
    uint8_t* result = malloc(length); FUN_ASSERT(result);
    fread(result, 1, length, f);
    fclose(f);
    return result;
}

char* fun_load_text(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* str = calloc(len+1, 1);
    fread(str, 1, len, f);
    fclose(f);
    return str;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Image parsing
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

bool fun_image_from_file(fun_image_t* restrict out, const char* path) {
    size_t      file_len = 0;
    uint8_t*    file_contents = NULL;
    if ((file_contents = fun_load_binary_file(path, &file_len)) == NULL || !file_len) {
        return false;
    }
    bool parsed = fun_image_from_memory(out, file_contents, file_len);
    free(file_contents);
    return parsed;
}

bool fun_image_from_memory(fun_image_t* restrict out, const uint8_t* buffer, size_t buffer_length) {
    spng_ctx* spng = spng_ctx_new(0); FUN_ASSERT(spng);
    spng_set_crc_action(spng, SPNG_CRC_USE, SPNG_CRC_USE);
    spng_set_png_buffer(spng, buffer, buffer_length);
    // Read header
    struct spng_ihdr hdr = { 0 };
    if (spng_get_ihdr(spng, &hdr) != 0) {
        fun_msg("Invalid PNG header\n");
        return false;
    }

    // Allocate output
    size_t data_len = sizeof(uint32_t) * hdr.width * hdr.height;
    out->width = hdr.width;
    out->height = hdr.height;
    out->data = malloc(data_len); FUN_ASSERT(out->data);

    // Decode
    int r = 0;
    if ((r = spng_decode_image(spng, out->data, data_len, SPNG_FMT_RGBA8, 0)) != 0) {
        fun_msg("Maformed PNG file returned %d)", r);
        return false;
    }

    return true;
}






