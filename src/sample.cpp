#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <ctime>
#include <immintrin.h>
#include <cassert>
#include <cstring>
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

#define pixel uint8_t
#define MAX_PB_SIZE 64
#define QPEL_EXTRA 7
#define QPEL_EXTRA_BEFORE 3
#define BIT_DEPTH 8
#define QPEL_FILTER(src, stride)                                               \
    (filter[0] * src[x - 3 * stride] +                                         \
     filter[1] * src[x - 2 * stride] +                                         \
     filter[2] * src[x -     stride] +                                         \
     filter[3] * src[x             ] +                                         \
     filter[4] * src[x +     stride] +                                         \
     filter[5] * src[x + 2 * stride] +                                         \
     filter[6] * src[x + 3 * stride] +                                         \
     filter[7] * src[x + 4 * stride])

const int8_t ff_hevc_qpel_filters[3][16] = {
    { -1,  4,-10, 58, 17, -5,  1,  0, -1,  4,-10, 58, 17, -5,  1,  0},
    { -1,  4,-11, 40, 40,-11,  4, -1, -1,  4,-11, 40, 40,-11,  4, -1},
    {  0,  1, -5, 17, 58,-10,  4, -1,  0,  1, -5, 17, 58,-10,  4, -1}
};

// const int16_t ff_hevc_qpel_filters_u16[3][16] = {
//     { -1,  4,-10, 58, 17, -5,  1,  0, -1,  4,-10, 58, 17, -5,  1,  0},
//     { -1,  4,-11, 40, 40,-11,  4, -1, -1,  4,-11, 40, 40,-11,  4, -1},
//     {  0,  1, -5, 17, 58,-10,  4, -1,  0,  1, -5, 17, 58,-10,  4, -1}
// };

const int16_t ff_hevc_qpel_filters_u16[3][16 * 8] = {
    { -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4,          -10, 58, -10, 58, -10, 58, -10, 58, -10, 58, -10, 58, -10, 58, -10, 58,               17, -5, 17, -5, 17, -5, 17, -5, 17, -5, 17, -5, 17, -5, 17, -5,          1,  0, 1,  0, 1,  0, 1,  0, 1,  0, 1,  0, 1,  0, 1,  0,        -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4,                      -10, 58, -10, 58, -10, 58, -10, 58, -10, 58, -10, 58, -10, 58, -10, 58,                  17, -5, 17, -5, 17, -5, 17, -5, 17, -5, 17, -5, 17, -5, 17, -5,       1,  0, 1,  0, 1,  0, 1,  0, 1,  0, 1,  0, 1,  0, 1,  0},
    { -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4,          -11, 40, -11, 40, -11, 40, -11, 40, -11, 40, -11, 40, -11, 40, -11, 40,               40,-11, 40,-11, 40,-11, 40,-11, 40,-11, 40,-11, 40,-11, 40,-11,          4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1,        -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4, -1,  4,                      -11, 40, -11, 40, -11, 40, -11, 40, -11, 40, -11, 40, -11, 40, -11, 40,                  40,-11, 40,-11, 40,-11, 40,-11, 40,-11, 40,-11, 40,-11, 40,-11,       4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1},
    {  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,           -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,               58,-10, 58,-10, 58,-10, 58,-10, 58,-10, 58,-10, 58,-10, 58,-10,          4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1,         0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,                       -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,  -5, 17,                  58,-10, 58,-10, 58,-10, 58,-10, 58,-10, 58,-10, 58,-10, 58,-10,       4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1, 4, -1}
};

static void put_hevc_qpel_hv(int16_t *dst,
                             uint8_t *_src,
                             ptrdiff_t _srcstride,
                             int height, intptr_t mx,
                             intptr_t my, int width)
{
    int x, y;
    const int8_t *filter;
    pixel *src = (pixel*)_src;
    ptrdiff_t srcstride = _srcstride / sizeof(pixel);
    int16_t tmp_array[(MAX_PB_SIZE + QPEL_EXTRA) * MAX_PB_SIZE];
    int16_t *tmp = tmp_array;

    src   -= QPEL_EXTRA_BEFORE * srcstride;
    filter = ff_hevc_qpel_filters[mx - 1];
    for (y = 0; y < height + QPEL_EXTRA; y++) {
        for (x = 0; x < width; x++)
            tmp[x] = QPEL_FILTER(src, 1) >> (BIT_DEPTH - 8);
        src += srcstride;
        tmp += MAX_PB_SIZE;
    }

    tmp    = tmp_array + QPEL_EXTRA_BEFORE * MAX_PB_SIZE;
    filter = ff_hevc_qpel_filters[my - 1];
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++)
            dst[x] = QPEL_FILTER(tmp, MAX_PB_SIZE) >> 6;
        tmp += MAX_PB_SIZE;
        dst += MAX_PB_SIZE;
    }
}

struct u8x16
{
    uint8_t data[32];
};

struct s8x16
{
    int8_t data[32];
};

struct s16x8
{
    int16_t data[8];
};

struct s16x16
{
    int16_t data[16];
};

struct s16x32
{
    int16_t data[32];
};

struct s32x8
{
    int32_t data[8];
};

struct s32x16
{
    int32_t data[16];
};

uint8_t pb_index[64] = {
     0,  1,  2,  3,
     1,  2,  3,  4,
     2,  3,  4,  5,
     3,  4,  5,  6,
     4,  5,  6,  7,
     5,  6,  7,  8,
     6,  7,  8,  9,
     7,  8,  9, 10,
     4,  5,  6,  7,
     5,  6,  7,  8,
     6,  7,  8,  9,
     7,  8,  9, 10,
     8,  9, 10, 11,
     9, 10, 11, 12,
    10, 11, 12, 13,
    11, 12, 13, 14
};

// uint16_t pw_index[64] = {
//      0, 16,  4, 20, 32, 48, 36, 52,
//      1, 17,  5, 21, 33, 49, 37, 53,
//      2, 18,  6, 22, 34, 50, 38, 54,
//      3, 19,  7, 23, 35, 51, 39, 55,
//      8, 24, 12, 28, 40, 56, 44, 60,
//      9, 25, 13, 29, 41, 57, 45, 61,
//     10, 26, 14, 30, 42, 58, 46, 62,
//     11, 27, 15, 31, 43, 59, 47, 63
// };

uint16_t pw_index[64] = {
     0, 16,  1, 17,  2, 18,  3, 19,  8, 24,  9, 25, 10, 26, 11, 27,
     4, 20,  5, 21,  6, 22,  7, 23, 12, 28, 13, 29, 14, 30, 15, 31,
    32, 48, 33, 49, 34, 50, 35, 51, 40, 56, 41, 57, 42, 58, 43, 59,
    36, 52, 37, 53, 38, 54, 39, 55, 44, 60, 45, 61, 46, 62, 47, 63
};

static void put_hevc_qpel_hv_vnni(int16_t *dst,
                             uint8_t *_src,
                             ptrdiff_t _srcstride,
                             int height, intptr_t mx,
                             intptr_t my, int width)
{
    int x, y;
    const int8_t *filter;
    pixel *src = (pixel*)_src;
    ptrdiff_t srcstride = _srcstride / sizeof(pixel);
    int16_t tmp_array[(MAX_PB_SIZE + QPEL_EXTRA) * MAX_PB_SIZE];
    int16_t *tmp = tmp_array;

    auto zero = _mm256_set1_epi8(0);
    src   -= QPEL_EXTRA_BEFORE * srcstride;
    filter = ff_hevc_qpel_filters[mx - 1];
    auto shuf0 = _mm256_loadu_epi8(pb_index);
    auto shuf1 = _mm256_loadu_epi8(pb_index + 32);

    auto shuf2 = _mm512_loadu_epi16(pw_index);
    auto shuf3 = _mm512_loadu_epi16(pw_index + 32);

    __m64 zero_m64;
    auto filter0 = _mm256_set1_epi32(*(int32_t *)ff_hevc_qpel_filters[mx - 1]);
    auto filter1 = _mm256_set1_epi32(*(int32_t *)(ff_hevc_qpel_filters[mx - 1] + 4));
    auto filter3 = _mm512_loadu_epi16(ff_hevc_qpel_filters_u16[my - 1]);
    auto filter4 = _mm512_loadu_epi16(ff_hevc_qpel_filters_u16[my - 1] + 32);
    // auto fv0 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][0]);
    // auto fv1 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][1]);
    // auto fv2 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][2]);
    // auto fv3 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][3]);
    // auto fv4 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][4]);
    // auto fv5 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][5]);
    // auto fv6 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][6]);
    // auto fv7 = _mm256_set1_epi32(ff_hevc_qpel_filters_u16[my - 1][7]);
    s8x16 *f0 = (s8x16 *)&filter0;
    s8x16 *f1 = (s8x16 *)&filter1;

#define H_LOAD_COMPUTE(n) \
        auto ymm##n     = _mm256_castsi128_si256(_mm_loadu_epi8(simd_src - 3)); \
        auto left##n    = _mm256_permutexvar_epi8(shuf0, ymm##n); \
        auto right##n   = _mm256_permutexvar_epi8(shuf1, ymm##n); \
        auto leftres##n = _mm256_dpbusd_epi32(zero, left##n, filter0); \
        auto res##n = _mm256_dpbusd_epi32(leftres##n, right##n, filter1); \
        simd_src += srcstride;

    uint8_t *simd_src = src;
    H_LOAD_COMPUTE(0)
    H_LOAD_COMPUTE(1)
    H_LOAD_COMPUTE(2)
    H_LOAD_COMPUTE(3)
    H_LOAD_COMPUTE(4)
    H_LOAD_COMPUTE(5)
    H_LOAD_COMPUTE(6)

    printf("x -> \n");
    for (y = 0; y < height + QPEL_EXTRA; y++) {
        for (x = 0; x < width; x++) {
            tmp[x] = QPEL_FILTER(src, 1) >> (BIT_DEPTH - 8);
        }
        printf("%d: \t", y);
        for (int i = 0; i < width; i++)
        {
            printf("%10d", tmp[i]);
        }
        putchar(10);
        src += srcstride;
        tmp += MAX_PB_SIZE;
    }

    printf("y -> \n");
    tmp    = tmp_array + QPEL_EXTRA_BEFORE * MAX_PB_SIZE;
    filter = ff_hevc_qpel_filters[my - 1];
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++)
            dst[x] = QPEL_FILTER(tmp, MAX_PB_SIZE) >> 6;
        printf("%d: \t", y);
        for (int i = 0; i < width; i++)
        {
            printf("%10d", dst[i]);
        }
        putchar(10);
        tmp += MAX_PB_SIZE;
        dst += MAX_PB_SIZE;
    }
    s16x16 *test0;
    s16x32 *test1;
    s32x8 *test3;
    s32x16 *test4;
    printf("simd x -> \n");
    int32_t *s32;
    s32 = ((s32x8 *)&res0)->data;
    printf("0: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    s32 = ((s32x8 *)&res1)->data;
    printf("1: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    s32 = ((s32x8 *)&res2)->data;
    printf("2: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    s32 = ((s32x8 *)&res3)->data;
    printf("3: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    s32 = ((s32x8 *)&res4)->data;
    printf("4: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    s32 = ((s32x8 *)&res5)->data;
    printf("5: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    s32 = ((s32x8 *)&res6)->data;
    printf("6: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    for (y = 0; y < height; y++) {
        H_LOAD_COMPUTE(7);
        auto n0 = _mm512_inserti64x4(_mm512_castsi256_si512(res0), res1, 1);
        auto n1 = _mm512_inserti64x4(_mm512_castsi256_si512(res2), res3, 1);
        auto n2 = _mm512_inserti64x4(_mm512_castsi256_si512(res4), res5, 1);
        auto n3 = _mm512_inserti64x4(_mm512_castsi256_si512(res6), res7, 1);
        auto w1 = _mm512_packs_epi32(n0, n1);
        auto w2 = _mm512_packs_epi32(n2, n3);
        auto a1 = _mm512_permutex2var_epi16(w1, shuf2, w2);
        auto a2 = _mm512_permutex2var_epi16(w1, shuf3, w2);
        a1 = _mm512_dpwssd_epi32(_mm512_set1_epi32(0), a1, filter3);
        a1 = _mm512_dpwssd_epi32(a1, a2, filter4);
        auto ret = _mm256_add_epi32(_mm512_extracti64x4_epi64(a1, 1), _mm512_castsi512_si256(a1));
        _mm_storeu_epi16(dst, _mm256_cvtepi32_epi16(_mm256_sra_epi32(ret, _mm_set1_epi64x(6))));
        // auto n0 = _mm256_mullo_epi32(res0, fv0);
        // auto n1 = _mm256_mullo_epi32(res1, fv1);
        // auto n2 = _mm256_mullo_epi32(res2, fv2);
        // auto n3 = _mm256_mullo_epi32(res3, fv3);
        // auto n4 = _mm256_mullo_epi32(res4, fv4);
        // auto n5 = _mm256_mullo_epi32(res5, fv5);
        // auto n6 = _mm256_mullo_epi32(res6, fv6);
        // auto n7 = _mm256_mullo_epi32(res7, fv7);
        // auto sum0 = _mm256_add_epi32(n0, n1);
        // auto sum1 = _mm256_add_epi32(n2, n3);
        // auto sum2 = _mm256_add_epi32(n4, n5);
        // auto sum3 = _mm256_add_epi32(n6, n7);
        // sum0 = _mm256_add_epi32(sum0, sum1);
        // sum1 = _mm256_add_epi32(sum2, sum3);
        // sum0 = _mm256_add_epi32(sum0, sum1);
        // _mm_storeu_epi16(dst, _mm256_cvtepi32_epi16(_mm256_sra_epi32(sum0, _mm_set1_epi64x(6))));

        res0 = res1;
        res1 = res2;
        res2 = res3;
        res3 = res4;
        res4 = res5;
        res5 = res6;
        res6 = res7;
        dst += MAX_PB_SIZE;
        s32 = ((s32x8 *)&res7)->data;
        printf("%d: \t%10d%10d%10d%10d%10d%10d%10d%10d\n", y + QPEL_EXTRA, s32[0], s32[1], s32[2], s32[3], s32[4], s32[5], s32[6], s32[7]);
    }

    printf("simd y -> \n");
    dst -= height * MAX_PB_SIZE;
    for (y = 0; y < height; y++) {
        printf("%d: \t", y);
        for (int i = 0; i < width; i++)
        {
            printf("%10d", dst[i]);
        }
        putchar(10);
        dst += MAX_PB_SIZE;
    }
}

int main()
{
    uint8_t coefficients[64] = {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };


    uint8_t safe_area[1024] = { 0 };
    memset(safe_area, 1, sizeof(safe_area));
    memcpy(safe_area + 512, coefficients, sizeof(coefficients));

    auto *ptr = safe_area + 512;


    int16_t dst[64 * 8] = { 0 };
    put_hevc_qpel_hv(dst, ptr, 8, 8, 1, 1, 8);
    put_hevc_qpel_hv_vnni(dst, ptr, 8, 8, 1, 1, 8);
    uint16_t out[64] = {
        0
    };

    // jpeg_zigzag_avx512bw(coefficients, out);

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

    int16_t a[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    int16_t b[] = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

    auto m1 = _mm512_loadu_epi16(&a);
    auto m2 = _mm512_loadu_epi16(&b);

    auto m3 = _mm512_set1_epi16(0);
    m3 = _mm512_dpwssds_epi32(m3, m1, m2);

    for (int i = 0; i < 16; i++)
    {
        printf("%4d", ((int *)&m3)[i]);
    }
    fflush(stdout);

    return 0;
}
