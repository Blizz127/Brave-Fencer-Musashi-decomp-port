/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct { s16 a, b, c; } V8014FC18;
typedef struct { s16 a, b, c; u8 lo; u8 hi; } W8014FC18;
extern s32 func_80133784(s32 k, V8014FC18 *p, W8014FC18 *q);

s32 func_8014FC18(u8 *o)
{
    V8014FC18 A;
    W8014FC18 B;
    s32 r;

    A.a = *(u16 *)(o + 6);
    A.b = *(u16 *)(o + 0xA);
    A.c = *(u16 *)(o + 0xE);
    B.a = *(u16 *)(o + 6);
    B.b = *(u16 *)(o + 0xA) + 0x10;
    B.c = *(u16 *)(o + 0xE);
    r = func_80133784(1, &A, &B);
    if (r != 0) {
        *(u16 *)(o + 6) = B.a;
        *(u16 *)(o + 0xA) = B.b;
        *(u16 *)(o + 0xE) = B.c;
        if (B.lo != 0) {
            *(u16 *)(o + 0x16C) = B.lo;
        }
        *(u16 *)(o + 0x16E) = *(u16 *)&B.lo >> 13;
        return r;
    }
    if (B.lo != 0) {
        *(u16 *)(o + 0x16C) = B.lo;
    }
    return 0;
}
