// SPDX-License-Identifier: MIT

#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Macros
//

#define Assert(...) assert(__VA_ARGS__)

//
// Fundamental types
//

typedef uint8_t     U8;
typedef uint16_t    U16;
typedef uint32_t    U32;
typedef uint64_t    U64;
typedef int8_t      S8;
typedef int16_t     S16;
typedef int32_t     S32;
typedef int64_t     S64;

typedef float       F32;
typedef double      F64;

typedef size_t      USize;

static inline U32 RotLeft32(U32 val, USize shift) {
    return (val << shift) | (val >> ((sizeof(U32) * 8) - shift));
}

//
// Structures
//

typedef struct Buffer {
    void* data;
    USize length;
} Buffer;

//
// Filesystem
//

static inline Buffer LoadBinaryFile(const char* path) {
    Buffer buffer = { 0, 0 };
    FILE* f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        buffer.length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer.data = malloc(buffer.length);
        fread(buffer.data, 1, buffer.length, f);
        fclose(f);
    }
    return buffer;
}

#endif // _COMMON_H_