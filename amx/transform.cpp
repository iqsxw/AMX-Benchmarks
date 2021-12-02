#include "transform.h"

#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <memory>
#include <immintrin.h>

extern "C" void amx_matrix_mul(void *cfg, void *a, void *b, void *c);
extern "C" void amx_matrix_mul_zero(void *cfg, void *a, void *b, void *c);

void calc_matrix_dpbuud(int *dst_buf, uint8_t *src1_buf, uint8_t *src2_buf)
{
    int M = 16; // rows
    int N = 64 / 4; // colsb
    int K = 64 / 4;
    int i, j, k, t;

    for (i = 0; i < M; i++)
        for (j = 0; j < N; j++)
        for (k = 0; k < K; k++)
        for (t = 0; t < 4; t++)
        {
            dst_buf[i * N + k] +=
            ((unsigned) src1_buf[i * 4 * N + 4 * j + t]) *
            ((unsigned) src2_buf[j * 4 * K + 4 * k + t]);

            // printf("dst[%d] +=  a[%d] * b[%d]\n", i * N + k, i * 4 * N + 4 * j + t, j * 4 * K + 4 * k + t);
        }
}

void calc_matrix_dpbssd(int *dst_buf, int8_t *src1_buf, int8_t *src2_buf)
{
    int M = 16; // rows
    int N = 64 / 4; // colsb
    int K = 64 / 4;
    int i, j, k, t;

    for (i = 0; i < M; i++)
        for (j = 0; j < N; j++)
        for (k = 0; k < K; k++)
        for (t = 0; t < 4; t++)
        {
            dst_buf[i * N + k] +=
            ((unsigned) src1_buf[i * 4 * N + 4 * j + t]) *
            ((unsigned) src2_buf[j * 4 * K + 4 * k + t]);

            // printf("dst[%d] +=  a[%d] * b[%d]\n", i * N + k, i * 4 * N + 4 * j + t, j * 4 * K + 4 * k + t);
        }
}

inline void ToB(uint8_t *src, uint8_t *dst)
{
    int cols = 16;
    int rows = 16;

    for (int k = 0; k < rows/4; k++)
    {
        for (int i = 0; i < cols; i ++)
        {
            for (int j = 0; j < rows/4; j++)
            {
                *dst++ = src[j * 16 + i];
            }
        }
        src += cols*4;
    }
}

inline void ToA(uint8_t *src, uint8_t *dst)
{
    for (int i = 0; i < 16; i++, dst += 64)
    {
        auto tmp128 = _mm_load_epi64(src + i*16);
        auto tmp512 = _mm512_broadcast_i64x2(tmp128);
        _mm512_store_epi64(dst, tmp512);
    }
}

static inline void AVX512ToUINT16(int32_t *data)
{
    auto additional = _mm512_set1_epi32(32767);

#define LOAD_AND_TO_UINT16(n) \
    auto tmp##n = _mm512_load_epi32(data + 16 * n);\
    tmp##n = _mm512_add_epi32(tmp##n, additional); \
    _mm512_store_epi32(data + 16 * n, tmp##n);

    LOAD_AND_TO_UINT16(0);
    LOAD_AND_TO_UINT16(1);
    LOAD_AND_TO_UINT16(2);
    LOAD_AND_TO_UINT16(3);
    LOAD_AND_TO_UINT16(4);
    LOAD_AND_TO_UINT16(5);
    LOAD_AND_TO_UINT16(6);
    LOAD_AND_TO_UINT16(7);
    LOAD_AND_TO_UINT16(8);
    LOAD_AND_TO_UINT16(9);
    LOAD_AND_TO_UINT16(10);
    LOAD_AND_TO_UINT16(11);
    LOAD_AND_TO_UINT16(12);
    LOAD_AND_TO_UINT16(13);
    LOAD_AND_TO_UINT16(14);
    LOAD_AND_TO_UINT16(15);
#undef LOAD_AND_TO_UINT16
}

static inline void AVX512ToUINT16_WITH_SHIFT(int32_t *data)
{
    auto additional = _mm512_set1_epi32(INT16_MAX);
    auto shift = _mm512_set1_epi32(3);

#define LOAD_AND_TO_UINT16(n) \
    auto tmp##n = _mm512_load_epi32(data + 16 * n);\
    tmp##n = _mm512_srav_epi32(tmp##n, shift); \
    tmp##n = _mm512_add_epi32(tmp##n, additional); \
    _mm512_store_epi32(data + 16 * n, tmp##n);

    LOAD_AND_TO_UINT16(0);
    LOAD_AND_TO_UINT16(1);
    LOAD_AND_TO_UINT16(2);
    LOAD_AND_TO_UINT16(3);
    LOAD_AND_TO_UINT16(4);
    LOAD_AND_TO_UINT16(5);
    LOAD_AND_TO_UINT16(6);
    LOAD_AND_TO_UINT16(7);
    LOAD_AND_TO_UINT16(8);
    LOAD_AND_TO_UINT16(9);
    LOAD_AND_TO_UINT16(10);
    LOAD_AND_TO_UINT16(11);
    LOAD_AND_TO_UINT16(12);
    LOAD_AND_TO_UINT16(13);
    LOAD_AND_TO_UINT16(14);
    LOAD_AND_TO_UINT16(15);
#undef LOAD_AND_TO_UINT16
}

void TestMatrixMultiply()
{
    uint8_t AMX_ALIGN(64) x[16*16] = {
        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    };

    int8_t AMX_ALIGN(64) a[1024] = { 0 };

    ToA((uint8_t *)HEVC::Transform, (uint8_t *)a);
    DisplayIntergerMatrix(HEVC::Transform, 16*16, "Before broadcast\n", 16);
    DisplayIntergerMatrix(a, 16*64, "After  broadcast\n", 16);

    int8_t AMX_ALIGN(64) y[16*16] = {
        0
    };

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            // y[j * 16 + i] = i * 16 + j;
            y[j*16+i] = -128;
        }
    }

    int8_t b[16*64] = { 0 };
    ToB((uint8_t *)y, (uint8_t *)b);
    DisplayIntergerMatrix(y, 16*16, "Coefficients Matrix\n", 16);
    DisplayIntergerMatrix(b, 16*64, "Converted Coefficients Matrix\n", 16);

    TileConfig cfg_src{
        0,
        { 64, 16, 64, 64, 64, 64, 64, 64 },
        { 16, 16, 4, 16, 16, 16, 16, 16 }
    };

    int AMX_ALIGN(64) c[256] = { 0 };
    int AMX_ALIGN(64) d[256] = { 0 };
    calc_matrix_dpbssd(c, a, b);
    amx_matrix_mul_zero(&cfg_src, a, b, d);
    DisplayIntergerMatrix(a, 1024, "A:\n", 32);
    DisplayIntergerMatrix(b, 1024, "B:\n", 32);
    DisplayIntergerMatrix(c, 256, "C(AMX C Emulation) = AB:\n", 16);
    DisplayIntergerMatrix(d, 256, "D(AMX Assembly) = AB:\n", 16);

    AVX512ToUINT16_WITH_SHIFT(c);
    AVX512ToUINT16_WITH_SHIFT(d);

    DisplayIntergerMatrix(c, 256, "C(AMX C Emulation) = AB:\n", 16);
    DisplayIntergerMatrix(d, 256, "D(AMX Assembly) = AB:\n", 16);

    return;
}

static inline void amx_set1_epi32(int32_t *dst, int16_t *a, int8_t *b, int32_t C)
{
    auto value = _mm512_set1_epi32(-1 * C);
#define STORE(n) \
    auto b8##n = _mm_load_epi64(b + 16 * n); \
    auto b##n = _mm512_cvtepi8_epi32(b8##n); \
    auto res##n = _mm512_mullo_epi32(b##n, value); \
    _mm512_store_epi32(dst + 16 * n, res##n);

    STORE(0);
    STORE(1);
    STORE(2);
    STORE(3);
    STORE(4);
    STORE(5);
    STORE(6);
    STORE(7);
    STORE(8);
    STORE(9);
    STORE(10);
    STORE(11);
    STORE(12);
    STORE(13);
    STORE(14);
    STORE(15);
#undef STORE
}

static inline void mm512_split_to_two_uint8(int16_t *src, uint8_t *hi, uint8_t *lo)
{
    auto additional = _mm512_set1_epi32(0x7fff7fff);
    auto low_mask = _mm512_set1_epi32(0xff00ff);
    auto shift = _mm512_set1_epi32(0x80008);

#define SPLIT(n) \
    auto tmp##n = _mm512_load_epi32(src + 32 * n); \
    tmp##n = _mm512_add_epi16(tmp##n, additional); \
    auto low_mm##n = _mm512_and_epi32(tmp##n, low_mask); \
    auto low_epi8_bit##n = _mm512_cvtepi16_epi8(low_mm##n); \
    _mm256_store_epi64(lo + 32 * n, low_epi8_bit##n); \
    auto high_mm##n = _mm512_srav_epi16(tmp##n, shift); \
    auto high_epi8_bit##n = _mm512_cvtepi16_epi8(high_mm##n); \
    _mm256_store_epi64(hi + 32 * n, high_epi8_bit##n);

    SPLIT(0);
    SPLIT(1);
    SPLIT(2);
    SPLIT(3);
    SPLIT(4);
    SPLIT(5);
    SPLIT(6);
    SPLIT(7);
#undef SHUFFLE
}

inline constexpr int32_t scale_factor(int bit_depth, int basic_vector_length)
{
    return (bit_depth + log2(basic_vector_length) - 9);
}

inline void mm512_lshift_add_rshift(int32_t *a, int32_t *b)
{
    auto lcount = _mm512_set1_epi32(8);
    auto rcount = _mm512_set1_epi32(scale_factor(16, 16));
#define SLA_ADD(n) \
    auto a##n = _mm512_load_epi32(a + 16 * n); \
    auto b##n = _mm512_load_epi32(b + 16 * n); \
    a##n = _mm512_sllv_epi32(a##n, lcount); \
    auto res##n = _mm512_add_epi32(a##n, b##n); \
    res##n = _mm512_srav_epi32(res##n, rcount); \
    _mm512_store_epi32(a + 16 * n, res##n);

    SLA_ADD(0);
    SLA_ADD(1);
    SLA_ADD(2);
    SLA_ADD(3);
    SLA_ADD(4);
    SLA_ADD(5);
    SLA_ADD(6);
    SLA_ADD(7);
    SLA_ADD(8);
    SLA_ADD(9);
    SLA_ADD(10);
    SLA_ADD(11);
    SLA_ADD(12);
    SLA_ADD(13);
    SLA_ADD(14);
    SLA_ADD(15);

#undef SLA_ADD
}

void amx_dct_16x16(int16_t *coefficients)
{
    TileConfig cfg_src{
        0,
        { 64, 16, 64, 64, 64, 64, 64, 64 },
        { 16, 16, 4, 16, 16, 16, 16, 16 }
    };

    int32_t AMX_ALIGN(64) c[256] = { 0 };
    int32_t AMX_ALIGN(64) d[256] = { 0 };

    uint8_t AMX_ALIGN(64) xh[256] = { 0 };
    uint8_t AMX_ALIGN(64) xl[256] = { 0 };

    uint8_t AMX_ALIGN(64) xh_b[1024] = { 0 };
    uint8_t AMX_ALIGN(64) xl_b[1024] = { 0 };

    mm512_split_to_two_uint8(coefficients, xh, xl);

    ToB(xh, xh_b);
    ToB(xl, xl_b);

    memcpy(d, HEVC::Sub, sizeof(HEVC::Sub));
    amx_matrix_mul_zero(&cfg_src, HEVC::Transform_1024, xh_b, c);
    amx_matrix_mul(&cfg_src, HEVC::Transform_1024, xl_b, d);
    mm512_lshift_add_rshift(c, d);

    // DisplayIntergerMatrix(c, 16 * 16, "Result\n", 16);

    return;
}
