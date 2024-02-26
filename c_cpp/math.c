// SPDX-License-Identifier: MIT

#include "util/math.h"

static inline void PrintMatrix(M4x4 m) {
    for (USize i = 0; i < 4; ++i) {
        Info("[%f, %f, %f, %f]", m.m[i*4+0], m.m[i*4+1], m.m[i*4+2], m.m[i*4+3]);
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

        M4x4 m4test = Mat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        Info("m4test:"); PrintMatrix(m4test);

        {
            M4x4 result = Mat4Mul(m4test, m4ident);
            Info("m4test * m4ident:"); PrintMatrix(result);
        }
    }

	return EXIT_FAILURE;
}