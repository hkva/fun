// SPDX-License-Identifier: MIT

#include "util/math.h"

#include "HandmadeMath.h"

static inline void PrintMatrix(M4x4 m) {
    for (USize i = 0; i < 4; ++i) {
        Info("[%f, %f, %f, %f]", m.m[i*4+0], m.m[i*4+1], m.m[i*4+2], m.m[i*4+3]);
    }
}

static inline void PrintMatrixHMM(HMM_Mat4 m) {
    for (USize i = 0; i < 4; ++i) {
        Info("[%f, %f, %f, %f]", m.Elements[i][0], m.Elements[i][1], m.Elements[i][2], m.Elements[i][3]);
    }
}

int main(int argc, const char* argv[]) {
	// V3
	{
		V3 v1 = Vec3(1.0f, 2.0f, 3.0f);
		V3 v2 = v1;
		V3 v3 = Vec3Add(v1, v2);
		Info("<1.0, 2.0, 3.0> + <1.0, 2.0, 3.0> = <%f, %f, %f>", v3.x, v3.y, v3.z);
		V3 v4 = Vec3Sub(v1, v2);
		Info("<1.0, 2.0, 3.0> - <1.0, 2.0, 3.0> = <%f, %f, %f>", v4.x, v4.y, v4.z);
	}

    // M4x4
    {
        M4x4 m4zero = Mat4Zero();
        Info("m4zero:"); PrintMatrix(m4zero);

        M4x4 m4ident = Mat4Ident();
        Info("m4ident:"); PrintMatrix(m4ident);

        {
            M4x4 m4_1 = Mat4(
                1, 2, 3, 4,
                5, 6, 7, 8,
                9, 10, 11, 12,
                13, 14, 15, 16
            );
            M4x4 m4_2 = Mat4(
                17, 18, 19, 20,
                21, 22, 23, 24,
                25, 26, 27, 28,
                29, 30, 31, 32
            );
            M4x4 m4_3 = Mat4Mul(m4_1, m4_2); Info("m4_1 * m4_2:"); PrintMatrix(m4_3);

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
            Info("hmm_m4_1 * hmm_m4_2:"); PrintMatrixHMM(hmm_m4_3);
        }

        {
            M4x4 m4_perspective = Mat4Perspective(0.1f, 10.0f, 16.0f / 9.0f, 90.0f);
            Info("m4_perspective:"); PrintMatrix(m4_perspective);

            HMM_Mat4 hmm_m4_perspective = HMM_TransposeM4(HMM_Perspective_RH_NO(Deg2Rad(90.0f), 16.0f / 9.0f, 0.1f, 10.0f));
            Info("hmm_m4_perspective:"); PrintMatrixHMM(hmm_m4_perspective);
        }
    }

	return EXIT_FAILURE;
}