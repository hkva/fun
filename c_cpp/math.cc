// SPDX-License-Identifier: MIT

#include "hk.hh"

#include "HandmadeMath.h"

using namespace hk;

static inline void print_matrix(const Mat4& m) {
    for (usize i = 0; i < 4; ++i) {
        dbglog("[%f, %f, %f, %f]", m[i*4+0], m[i*4+1], m[i*4+2], m[i*4+3]);
    }
}

static inline void print_matrix_hmm(const HMM_Mat4& m) {
    for (usize i = 0; i < 4; ++i) {
        dbglog("[%f, %f, %f, %f]", m.Elements[i][0], m.Elements[i][1], m.Elements[i][2], m.Elements[i][3]);
    }
}

int main(int argc, const char* argv[]) {
	// V3
	{
		Vec3 v1 = Vec3(1.0f, 2.0f, 3.0f);
		Vec3 v2 = v1;
        Vec3 v3 = v1 + v2;
		dbglog("<1.0, 2.0, 3.0> + <1.0, 2.0, 3.0> = <%f, %f, %f>", v3.x, v3.y, v3.z);
		Vec3 v4 = v1 - v2;
		dbglog("<1.0, 2.0, 3.0> - <1.0, 2.0, 3.0> = <%f, %f, %f>", v4.x, v4.y, v4.z);
	}

    // M4x4
    {
        Mat4 m4zero = Mat4();
        dbglog("m4zero:"); print_matrix(m4zero);

        Mat4 m4ident = Mat4::ident();
        dbglog("m4ident:"); print_matrix(m4ident);

        {
            Mat4 m4_1 = Mat4(
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            );
            Mat4 m4_2 = Mat4(
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            );
            Mat4 m4_3 = m4_1 * m4_2; dbglog("m4_1 * m4_2:"); print_matrix(m4_3);

            HMM_Mat4 hmm_m4_1 = {{
                { 1, 2, 3, 4 },
                { 5, 6, 7, 8 },
                { 9, 10, 11, 12 },
                { 13, 14, 15, 16 }
            }};
            HMM_Mat4 hmm_m4_2 = {{
                { 17, 18, 19, 20 },
                { 21, 22, 23, 24 },
                { 25, 26, 27, 28 },
                { 29, 30, 31, 32 }
            }};
            HMM_Mat4 hmm_m4_3 = HMM_MulM4(hmm_m4_1, hmm_m4_2);
            dbglog("hmm_m4_1 * hmm_m4_2:"); print_matrix_hmm(hmm_m4_3);
        }

        {
            Mat4 m4_perspective = Mat4::perspective(0.1f, 10.0f, 16.0f / 9.0f, 90.0f);
            dbglog("m4_perspective:"); print_matrix(m4_perspective);

            HMM_Mat4 hmm_m4_perspective = HMM_TransposeM4(HMM_Perspective_RH_NO(deg2rad(90.0f), 16.0f / 9.0f, 0.1f, 10.0f));
            dbglog("hmm_m4_perspective:"); print_matrix_hmm(hmm_m4_perspective);
        }
    }

    // RNG
    {
        RandomXOR r = RandomXOR();

        for (usize i = 0; i < 20; ++i) {
            dbglog("xorshift f32 [0, 1]: %f", r.random<f32>(0.0f, 1.0f));
        }

        for (usize i = 0; i < 20; ++i) {
            dbglog("xorshift u32 [0, 100]: %u", r.random<u32>(0, 100));
        }
    }

	return EXIT_FAILURE;
}
