#ifndef _UTIL_MATH_H_
#define _UTIL_MATH_H_

#include <math.h>

#include "common.h"

//
// Geometry
//

#ifndef PI
#   define PI 3.1415926535f
#endif

static inline F32 Deg2Rad(F32 deg) {
    return deg * PI / 180.0f;
}

static inline F32 Rad2Deg(F32 rad) {
    return rad * 180.0f / PI;
}

#define Sin sinf
#define Cos cosf
#define Tan tanf

//
// Vector2
//

typedef struct V2 {
    F32 x;
    F32 y;
} V2;

#define Vec2(x, y) ((V2){x, y})

//
// Vector3
//

typedef struct V3 {
    F32 x;
    F32 y;
    F32 z;
} V3;

#define Vec3(x, y, z) ((V3){x, y, z})

static inline V3 Vec3Zero(void) {
    return Vec3(0.0f, 0.0f, 0.0f);
}

static inline V3 Vec3Add(V3 left, V3 right) {
    return Vec3(left.x + right.x, left.y + right.y, left.z + right.z);
}

static inline V3 Vec3Sub(V3 left, V3 right) {
    return Vec3(left.x - right.x, left.y - right.y, left.z - right.z);
}

static inline F32 Vec3Dot(V3 left, V3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

static inline V3 Vec3Cross(V3 left, V3 right) {
    return Vec3(
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    );
}

static inline V3 Vec3Invert(V3 v) {
    return Vec3(-v.x, -v.y, -v.z);
}

//
// Matrix4x4
//

typedef struct M4x4 {
    float m[4*4];
} M4x4;

#define Mat4(...) ((M4x4){ { __VA_ARGS__ } })

#define Mat4Zero() Mat4(    \
    0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 0.0f, \
)

#define Mat4Ident() Mat4(   \
    1.0f, 0.0f, 0.0f, 0.0f, \
    0.0f, 1.0f, 0.0f, 0.0f, \
    0.0f, 0.0f, 1.0f, 0.0f, \
    0.0f, 0.0f, 0.0f, 1.0f, \
)

static inline M4x4 Mat4Mul(M4x4 left, M4x4 right) {
    M4x4 result = Mat4Zero();
    for (U8 i = 0; i < 4; ++i) {
        for (U8 j = 0; j < 4; ++j) {
            for (U8 k = 0; k < 4; ++k) {
                result.m[i * 4 + j] += left.m[i * 4 + k] * right.m[k * 4 + j];
            }
        }
    }
    return result;
}

static inline M4x4 Mat4Translate(V3 translate) {
    M4x4 result = Mat4Ident();
    result.m[0*4+3] = translate.x;
    result.m[1*4+3] = translate.y;
    result.m[2*4+3] = translate.z;
    return result;
}

static inline M4x4 Mat4Scale(V3 scale) {
    M4x4 result = Mat4Zero();
    result.m[0*4+0] = scale.x;
    result.m[1*4+1] = scale.y;
    result.m[2*4+2] = scale.z;
    result.m[3*4+3] = 1.0f;
    return result;
}

static inline M4x4 Mat4ScaleUniform(F32 scale) {
    M4x4 result = Mat4Zero();
    result.m[0*4+0] = scale;
    result.m[1*4+1] = scale;
    result.m[2*4+2] = scale;
    result.m[3*4+3] = 1.0f;
    return result;
}

static inline M4x4 Mat4RotateX(F32 degrees) {
    M4x4 result = Mat4Ident();
    const F32 theta = Deg2Rad(degrees);
    result.m[1*4+1] = Cos(theta);
    result.m[1*4+2] = Sin(theta);
    result.m[2*4+1] = -Sin(theta);
    result.m[2*4+2] = Cos(theta);
    return result;
}

static inline M4x4 Mat4RotateY(F32 degrees) {
    M4x4 result = Mat4Ident();
    const F32 theta = Deg2Rad(degrees);
    result.m[0*4+0] = Cos(theta);
    result.m[0*4+2] = -Sin(theta);
    result.m[2*4+0] = Sin(theta);
    result.m[2*4+2] = Cos(theta);
    return result;
}

static inline M4x4 Mat4Perspective(F32 near, F32 far, F32 aspect, F32 fov) {
    M4x4 result = Mat4Zero();

    const F32 tangent = Tan(Deg2Rad(fov / 2));
    const F32 half_height = near * tangent;
    const F32 half_width = half_height * aspect;

    result.m[0*4+0] = near / half_width;
    result.m[1*4+1] = near / half_height;
    result.m[2*4+2] = -1.0f * (far + near) / (far - near);
    result.m[2*4+3] = -2.0f * (far * near) / (far - near);
    result.m[3*4+2] = -1.0f;

    return result;
}

#endif // _UTIL_MATH_H_