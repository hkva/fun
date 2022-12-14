#ifndef _FUN_H_
#define _FUN_H_

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Compiler/platform options
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef __GNUC__
    // Subtract specific annoying warnings from -Wall
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#ifdef _MSC_VER
    // Same with MSVC
    #pragma warning(disable: 4100)
#endif
#ifdef __linux__
    #define FUN_LINUX
#endif

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Other headers
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Macros
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define FUN_ASSERT(cond) assert(cond)
#define FUN_ARRLEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#define FUN_ZERO(var)   memset(&var, 0, sizeof(var))

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Platform-specific stuff
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

char*   fun_basename(const char* path);
void    fun_chdir(const char* path);
void    fun_err(const char* fmt, ...);
void    fun_msg(const char* fmt, ...);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Math
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define FUN_PI 3.1415926535f

#define FUN_DEG2RAD(deg) ((deg) * FUN_PI / 180.0f)
#define FUN_RAD2DEG(rad) ((rad) * 180.0f / FUN_PI)

void fun_mat4_ident(float* restrict m);
void fun_mat4_mul(float* restrict dst, const float* restrict l, const float* restrict r);
void fun_mat4_perspective(float* restrict m, float fovy_rad, float aspect, float z_near, float z_far);
void fun_mat4_rotate(float* restrict m, float rad, float x, float y, float z);
void fun_mat4_orthographic(float* restrict m, float l, float r, float t, float b, float n, float f);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// IO
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

uint8_t*    fun_load_binary_file(const char* path, size_t* length);
char*       fun_load_text(const char* path);

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Image parsing
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// RGBA8 image
typedef struct fun_image {
    int         width;
    int         height;
    uint32_t*   data;
} fun_image_t;

bool fun_image_from_file(fun_image_t* restrict out, const char* path);
bool fun_image_from_memory(fun_image_t* restrict out, const uint8_t* buffer, size_t buffer_length);

#endif // _FUN_H_
