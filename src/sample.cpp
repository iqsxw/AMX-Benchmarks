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

int main()
{
    verify_fomula(-127, -9);
    test_performance();

    int16_t AMX_ALIGN(64) coefficients[16 * 16] = {0};

    for (int i = 0; i < 16 * 16; i++)
    {
        coefficients[i] = -32767;
    }

    DisplayIntergerMatrix(coefficients, 16*16, "\nTest with coefficients:\n", 16);

    amx_dct_16x16(coefficients);

    return 0;
}
