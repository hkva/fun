// SPDX-License-Identifier: MIT

// https://en.wikipedia.org/wiki/MD5
// https://github.com/B-Con/crypto-algorithms/blob/master/md5.c

#include "hk.hh"

using namespace hk;

static const u32 MD5_SHIFT_TABLE[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

static const u32 MD5_SIN_TABLE[64] = {
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
    0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
    0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
    0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
    0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
    0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
    0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
    0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
    0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: md5 <file>\n");
        return EXIT_FAILURE;
    }

    std::vector<u8> file = load_binary_file(argv[1]);
    if (!file.size()) {
       fprintf(stderr, "Failed to load file %s\n", argv[1]);
        return EXIT_FAILURE; 
    }

    // The MD5 algorithm expects data in 64-byte blocks. Data should be followed immediately by a "one" bit, then
    // padded with zeroes until the last 8 bytes of the last block, where the size of the input in bytes is written.
    // 
    // https://www.desmos.com/calculator/hypjdhc7v7
    const usize input_len = (((file.size() + 8) / 64) + 1) * 64;
    std::vector<u8> input = std::vector<u8>(); input.resize(input_len);

    // Set initial buffer
    memcpy(&input[0], &file[0], file.size());

    // Set "one" bit
    input[file.size()] = 1 << 7;

    // Write size in bite
    const u64 size_bits = file.size() * 8;
    memcpy(&input[input_len - sizeof(u64)], &size_bits, sizeof(size_bits));

    // Set initial state
    u32 state_A = 0x67452301;
    u32 state_B = 0xEFCDAB89;
    u32 state_C = 0x98BADCFE;
    u32 state_D = 0x10325476;

    // Process 512-bit chunks
    HK_ASSERT(input_len % 64 == 0);
    for (usize i = 0; i < input_len / 64; ++i) {
        u32 a = state_A;
        u32 b = state_B;
        u32 c = state_C;
        u32 d = state_D;

        // https://en.wikipedia.org/wiki/MD5#Algorithm
        #define F(b, c, d) ((b & c) | (~b & d))
        #define G(b, c, d) ((b & d) | (c & ~d))
        #define H(b, c, d) (b ^ c ^ d)
        #define I(b, c, d) (c ^ (b | ~d))

        for (u32 j = 0; j < 64; ++j) {
            u32 f = 0;
            u32 g = 0;
            if (j < 16) {
                f = F(b, c, d);
                g = j;
            }
            else if (j < 32) {
                f = G(b, c, d);
                g = (j * 5 + 1) % 16;
            }
            else if (j < 48) {
                f = H(b, c, d);
                g = (j * 3 + 5) % 16;
            }
            else {
                f = I(b, c, d);
                g = (j * 7) % 16;
            }
            f = f + a + MD5_SIN_TABLE[j] + ((u32*)&input[i * 64])[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, MD5_SHIFT_TABLE[j]);
        }

        #undef I
        #undef H
        #undef G
        #undef F

        state_A += a;
        state_B += b;
        state_C += c;
        state_D += d;
    }

    u8 digest[16];

    const u32 result32[4] = {
        state_A,
        state_B,
        state_C,
        state_D,
    };
    memcpy(digest, result32, sizeof(digest));

    fprintf(stderr, "%s %x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x\n", argv[1],
        digest[ 0], digest[ 1], digest[ 2], digest[ 3],
        digest[ 4], digest[ 5], digest[ 6], digest[ 7],
        digest[ 8], digest[ 9], digest[10], digest[11],
        digest[12], digest[13], digest[14], digest[15]);

    return EXIT_SUCCESS;
}