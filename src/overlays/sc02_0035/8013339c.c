/* Owner re-derivation from retail assembly (clean input set). */
/* Unscale a 3x3 matrix's columns by a packed s16 scale vector (x/y/z, zero
 * scale treated as 1): col0={m0,m3,m6}/=sx, col1={m1,m4,m7}/=sy,
 * col2={m2,m5,m8}/=sz, each numerator shifted left 12 (fixed point) first.
 * Each store happens right after its own division (not batched at the end). */
#include "psx_types.h"

typedef struct {
    s16 m0, m1, m2, m3, m4, m5, m6, m7, m8;
} Mat3x3;

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} Scale16;

void func_8013339C(Mat3x3 *mtx, Scale16 *scale)
{
    s16 sx_in;
    s16 sy_in;
    s16 sz_in;
    s16 sx;
    s16 sy;
    s16 sz;

    sx_in = scale->x;
    sx = 1;
    if (sx_in != 0) {
        sx = sx_in;
    }
    sy_in = scale->y;
    sy = 1;
    if (sy_in != 0) {
        sy = sy_in;
    }
    sz_in = scale->z;
    sz = 1;
    if (sz_in != 0) {
        sz = sz_in;
    }

    mtx->m0 = (s16)((s32)(mtx->m0 << 0xC) / sx);
    mtx->m3 = (s16)((s32)(mtx->m3 << 0xC) / sx);
    mtx->m6 = (s16)((s32)(mtx->m6 << 0xC) / sx);
    mtx->m1 = (s16)((s32)(mtx->m1 << 0xC) / sy);
    mtx->m4 = (s16)((s32)(mtx->m4 << 0xC) / sy);
    mtx->m7 = (s16)((s32)(mtx->m7 << 0xC) / sy);
    mtx->m2 = (s16)((s32)(mtx->m2 << 0xC) / sz);
    mtx->m5 = (s16)((s32)(mtx->m5 << 0xC) / sz);
    mtx->m8 = (s16)((s32)(mtx->m8 << 0xC) / sz);
}
