// SPDX-License-Identifier: MIT

#ifndef _HK_HH_
#define _HK_HH_

#include <cmath>
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <string> // @@ replace
#include <vector> // @@ replace

// ==============================
// Macros
// ==============================

#ifdef __GNUC__
#   define HK_GCC
#endif

#ifndef HK_ASSERT
#   define HK_ASSERT assert
#endif

#ifdef HK_GCC
#   define HK_PRINTF(fidx, vidx) __attribute__((format(printf, fidx, vidx)))
#else
#   define HK_PRINTF(fidx, vidx)
#endif

namespace hk {

// ==============================
// Fixed-size types
// ==============================

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using usize = std::size_t;

template <typename T, usize size>
static inline constexpr usize arrlen(T(&)[size]) {
    return size;
}

template <typename T>
static inline T rotate_left(T inp, usize shift) {
    return (inp << shift) | (inp >> ((sizeof(T) * 8) - shift));
}

template <typename T>
static inline constexpr T min(T x1, T x2) {
    return (x1 > x2) ? x2 : x1;
}

template <typename T>
static inline constexpr T max(T x1, T x2) {
    return (x1 > x2) ? x1 : x2;
}

template <typename T>
static inline constexpr bool inrange(T val, T lower, T upper) {
    return val >= lower && val <= upper;
}

// ==============================
// Debugging
// ==============================

HK_PRINTF(1, 2)
static inline void dbgerr(const char* fmt, ...) {
    std::va_list va; va_start(va, fmt);
    std::vprintf(fmt, va);
    va_end(va);
    std::printf("\n");

    std::abort();
}

HK_PRINTF(1, 2)
static inline void dbglog(const char* fmt, ...) {
    std::va_list va; va_start(va, fmt);
    std::vprintf(fmt, va);
    va_end(va);
    std::printf("\n");
}

// ==============================
// Math
// ==============================

constexpr f32 PI = 3.1415926535f;

static inline f32 deg2rad(const f32 rad) {
    return rad * PI / 180.0f;   
}

class Vec2 {
public:
    f32 x;
    f32 y;
public:
    Vec2() = default;

    Vec2(f32 x, f32 y) : x(x), y(y) {
    }

    Vec2 operator+(const Vec2& rhs) const {
        return Vec2(x + rhs.x, y + rhs.y);
    }

    Vec2 operator-(const Vec2& rhs) const {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    Vec2 scale(f32 scale) const {
        return Vec2(x * scale, y * scale);
    }
};

class Vec3 {
public:
    f32 x;
    f32 y;
    f32 z;
public:
    Vec3() = default;

    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {
    }

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3 invert() const {
        return Vec3(-x, -y, -z);
    }
public:
    static inline Vec3 broadcast(f32 val) {
        return Vec3(val, val, val);
    }
};

class Vec4 {
public:
union {
struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};
struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
};
};
public:
    Vec4() = default;

    Vec4(f32 x, f32 y, f32 z, f32 w = 1.0f) : x(x), y(y), z(z), w(w) {
    }
};

class Mat4 {
private:
    f32 m[4 * 4];
public:
    Mat4() = default;

    Mat4(
        f32 m11, f32 m12, f32 m13, f32 m14,
        f32 m21, f32 m22, f32 m23, f32 m24,
        f32 m31, f32 m32, f32 m33, f32 m34,
        f32 m41, f32 m42, f32 m43, f32 m44
    ) {
        m[ 0] = m11; m[ 1] = m12; m[ 2] = m13; m[ 3] = m14;
        m[ 4] = m21; m[ 5] = m22; m[ 6] = m23; m[ 7] = m24;
        m[ 8] = m31; m[ 9] = m32; m[10] = m33; m[11] = m34;
        m[12] = m41; m[13] = m42; m[14] = m43; m[15] = m44;
    }

    f32& operator[](usize idx) {
        return m[idx];
    }

    const f32& operator[](usize idx) const {
        return m[idx];
    }

    Mat4 operator*(const Mat4& rhs) const {
        // @@ optimize
        Mat4 result = Mat4();
        for (u8 i = 0; i < 4; ++i) {
            for (u8 j = 0; j < 4; ++j) {
                for (u8 k = 0; k < 4; ++k) {
                    result.m[i * 4 + j] += this->m[k * 4 + j] * rhs[i * 4 + k];
                }
            }
        }
        return result;
    }

    const f32* base() const { return m; }
public:
    static inline Mat4 ident() {
        Mat4 m = Mat4();
        m[0*4+0] = 1.0f;
        m[1*4+1] = 1.0f;
        m[2*4+2] = 1.0f;
        m[3*4+3] = 1.0f;
        return m;
    }

    static inline Mat4 translate(Vec3 translate) {
        Mat4 result = Mat4::ident();
        result[0*4+3] = translate.x;
        result[1*4+3] = translate.y;
        result[2*4+3] = translate.z;
        return result;
    }

    static inline Mat4 scale(Vec3 scale) {
        Mat4 result = Mat4::ident();
        result[0*4+0] = scale.x;
        result[1*4+1] = scale.y;
        result[2*4+2] = scale.z;
        return result;
    }

    static inline Mat4 rotate_x(f32 degrees) {
        Mat4 result = Mat4::ident();
        const f32 theta = deg2rad(degrees);
        result[1*4+1] = cos(theta);
        result[1*4+2] = sin(theta);
        result[2*4+1] = -sin(theta);
        result[2*4+2] = cos(theta);
        return result;
    }

    static inline Mat4 rotate_y(f32 degrees) {
        Mat4 result = Mat4::ident();
        const f32 theta = deg2rad(degrees);
        result[0*4+0] = cos(theta);
        result[0*4+2] = -sin(theta);
        result[2*4+0] = sin(theta);
        result[2*4+2] = cos(theta);
        return result;
    }

    static inline Mat4 perspective(f32 z_near, f32 z_far, f32 aspect, f32 fov) {
        Mat4 result = Mat4();

        const f32 cotangent = tan(deg2rad(fov / 2.0f));
        const f32 half_height = z_near * cotangent;
        const f32 half_width = half_height * aspect;

        result[0*4+0] = z_near / half_width;
        result[1*4+1] = z_near / half_height;
        result[2*4+2] = -1.0f * (z_far + z_near) / (z_far - z_near);
        result[2*4+3] = -2.0f * (z_far * z_near) / (z_far - z_near);
        result[3*4+2] = -1.0f;

        return result;
    }

    static inline Mat4 orthographic(f32 z_near, f32 z_far, f32 left, f32 right, f32 top, f32 bottom) {
        Mat4 result = Mat4();

        result[0*4+0] = 2.0f / (right - left);
        result[1*4+1] = 2.0f / (top - bottom);
        result[2*4+2] = 2.0f / (z_near - z_far);
        result[3*4+3] = 1.0f;

        result[0*4+3] = (left + right) / (left - right);
        result[1*4+3] = (bottom + top) / (bottom - top);
        result[2*4+3] = (z_near + z_far) / (z_near - z_far);

        return result;
    }
};

// ==============================
// String utilities
// ==============================

namespace str {

static inline char tolower(char c) {
    return inrange(c, 'A', 'Z') ? (c + ('a' - 'A')) : c;
}

static inline bool ieq(const char* s1, const char* s2) {
    for (usize i = 0; ; ++i) {
        if (tolower(s1[i]) != tolower(s2[i])) {
            return false;
        }
        if (s1[i] == '\0') {
            break;
        }
    }
    return true;
}

}

// ==============================
// RNG
// ==============================

class RandomXOR {
private:
    u32 state;
public:
    RandomXOR(u32 state = 0x12345678) : state(state) { }

    u32 next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    f32 next_unit() {
        return (f32)next() / (f32)0xFFFFFFFF;
    }

    template <typename T>
    T random(T lower, T upper) {
        return (T)(next_unit() * (T)(upper - lower)) + lower;
    }
};

// ==============================
// I/O
// ==============================

// @@ care about alloc+copy on return?
static inline std::vector<u8> load_binary_file(const char* path) {
    std::vector<u8> result = std::vector<u8>();
    std::FILE* f = std::fopen(path, "rb");
    if (f != nullptr) {
        std::fseek(f, 0, SEEK_END);
        result.resize(std::ftell(f));
        std::fseek(f, 0, SEEK_SET);
        std::fread(&result[0], 1, result.size(), f);
        std::fclose(f);
    }
    return result;
}

// @@
static inline std::string load_text_file(const char* path) {
    std::string result = std::string();
    std::FILE* f = std::fopen(path, "rb");
    if (f != nullptr) {
        std::fseek(f, 0, SEEK_END);
        result.resize(std::ftell(f));
        std::fseek(f, 0, SEEK_SET);
        std::fread(&result[0], 1, result.size(), f);
        std::fclose(f);
    }
    return result;
}

}

#endif // _HK_HH_