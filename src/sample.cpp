#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <ctime>
#include <immintrin.h>
#include <cassert>

#include "transform.h"

extern "C"  void ff_hevc_idct_16x16_8_avx(int16_t *coeff, int limit);

bool verify_fomula(int8_t a, int8_t b)
{
    int32_t c = a * b;

    uint16_t x = (a + INT16_MAX);
    int8_t y = b;

    uint8_t xh = x >> 8;
    uint8_t xl = x & 0xff;
    int32_t z  = ((y * xh) << 8) + (y * xl);

    uint16_t d = z - INT16_MAX * b;

    printf("a + b = \"%d\"\n", - INT16_MAX * b);
    assert(c == d && "Incorrect Calculation\n");

    return true;
}

void test_performance()
{
    int16_t AMX_ALIGN(64) data[16*16];

    int8_t a[16*64];
    int8_t b[16*64];
    int32_t c[16*16];

    {
        Timer timer{ "ff_hevc_idct_16x16_8_avx: " };
        for (volatile int i = 0; i < 10000; i++)
        {
            ff_hevc_idct_16x16_8_avx(data, 16);
        }

        ff_hevc_idct_16x16_8_avx(data, 16);
    }

    {
        Timer timer{ "AMX DCT_16X16: " };
        for (volatile int i = 0; i < 10000; i++)
        {
            amx_dct_16x16(data);
        }
        amx_dct_16x16(data);
    }
}

// int main()
// {
//     verify_fomula(-127, -9);
//     test_performance();

//     int16_t AMX_ALIGN(64) coefficients[16 * 16] = {0};

//     for (int i = 0; i < 16 * 16; i++)
//     {
//         coefficients[i] = -32767;
//     }

//     DisplayIntergerMatrix(coefficients, 16*16, "\nTest with coefficients:\n", 16);

//     amx_dct_16x16(coefficients);

//     return 0;
// }


#include <iostream>
#include <immintrin.h>

uint16_t zigzag_shuffle[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

void jpeg_zigzag_avx512bw(const uint16_t* in, uint16_t* out) {

    const __m512i A = _mm512_loadu_si512((const __m512i*)(in + 0 * 32));
    const __m512i B = _mm512_loadu_si512((const __m512i*)(in + 1 * 32));

    // note: zigzag_shuffle refers here to uint16_t array
    const __m512i shuf0 = _mm512_loadu_si512((const __m512i*)(zigzag_shuffle + 0 * 32));
    const __m512i res0 = _mm512_permutex2var_epi16(A, shuf0, B);
    const __m512i shuf1 = _mm512_loadu_si512((const __m512i*)(zigzag_shuffle + 1 * 32));
    const __m512i res1 = _mm512_permutex2var_epi16(A, shuf1, B);

    _mm512_storeu_si512((__m512i*)(out + 0 * 32), res0);
    _mm512_storeu_si512((__m512i*)(out + 1 * 32), res1);
}

int main()
{
    uint16_t coefficients[64] = {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };

    uint16_t out[64] = {
        0
    };

    jpeg_zigzag_avx512bw(coefficients, out);

    const int __attribute__((aligned(64))) jpeg_natural_order[64 + 16] = {
        0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63,
        63, 63, 63, 63, 63, 63, 63, 63, /* extra entries for safety in decoder */
        63, 63, 63, 63, 63, 63, 63, 63
        };

    uint16_t out2[64] = { 0 };
    for (int i = 0; i < 64; i++)
    {
        out2[i] = coefficients[jpeg_natural_order[i]];
        assert (out2[i] == out[i] && "break");
    }

    return 0;
}
