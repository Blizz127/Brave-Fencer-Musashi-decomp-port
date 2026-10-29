/* Owner re-derivation from retail assembly (clean input set). */
/* Fills the two 3-vectors D_80126AF0 / D_80126AF8 for mode k (0/1 from the +0x20 owner's helpers, 2 a heading-derived pair, otherwise a signed-byte table row selected by *cursor), steps the cursor unless k has the 0xF000000 bits, then runs func_80156ECC. */
#include "psx_types.h"

extern s16 D_801270A8[3];
extern u8 D_80126AF0[];
extern s32 func_800139C8(s32 a, void *b, void *c);
extern void func_801484B0(void *a, void *b);
extern void func_801484E8(void *a, void *b);
extern void func_80148534(void *a, void *b);
extern void func_8014856C(void *a, void *b);
extern void func_80156ECC(void *o, void *a, void *b, void *c, void *d);

void func_80156B74(u8 *o, u32 k, u8 *cur) {
    u8 *q;
    u16 a[3];
    u16 b[3];
    u32 e;
    u8 *base;
    q = *(u8 **)(o + 0x20);
    D_801270A8[0] = 0;
    D_801270A8[1] = -0x1E;
    D_801270A8[2] = 0;
    if (k == 0) {
        if (*(u32 *)(q + 0x20) != 0) {
            func_801484B0(a, b);
            ((s16 *)&D_80126AF0)[0] = a[0];
            ((s16 *)&D_80126AF0)[1] = a[1];
            ((s16 *)&D_80126AF0)[2] = a[2];
            func_801484E8(a, b);
            ((s16 *)&D_80126AF0)[4] = a[0];
            ((s16 *)&D_80126AF0)[5] = a[1];
            ((s16 *)&D_80126AF0)[6] = a[2];
        } else {
            ((s16 *)&D_80126AF0)[0] = 0;
            ((s16 *)&D_80126AF0)[1] = 0;
            ((s16 *)&D_80126AF0)[2] = 0;
            ((s16 *)&D_80126AF0)[4] = 0;
            ((s16 *)&D_80126AF0)[5] = 0;
            ((s16 *)&D_80126AF0)[6] = 0;
        }
    } else if (k == 1) {
        if (*(u32 *)(q + 0x20) != 0) {
            func_80148534(a, b);
            ((s16 *)&D_80126AF0)[0] = a[0];
            ((s16 *)&D_80126AF0)[1] = a[1];
            ((s16 *)&D_80126AF0)[2] = a[2];
            func_8014856C(a, b);
            ((s16 *)&D_80126AF0)[4] = a[1];
            ((s16 *)&D_80126AF0)[5] = a[1];
            ((s16 *)&D_80126AF0)[6] = a[2];
        } else {
            ((s16 *)&D_80126AF0)[0] = 0;
            ((s16 *)&D_80126AF0)[1] = 0;
            ((s16 *)&D_80126AF0)[2] = 0;
            ((s16 *)&D_80126AF0)[4] = 0;
            ((s16 *)&D_80126AF0)[5] = 0;
            ((s16 *)&D_80126AF0)[6] = 0;
        }
    } else if (k == 2) {
        ((s16 *)&D_80126AF0)[0] = 0;
        ((s16 *)&D_80126AF0)[1] = -0x1E;
        ((s16 *)&D_80126AF0)[2] = -0x52;
        func_800139C8((s16)(*(u16 *)(o + 0x24A) - *(u16 *)(*(u8 **)(o + 0x20) + 0x12)), ((s16 *)&D_80126AF0), ((s16 *)&D_80126AF0));
        ((s16 *)&D_80126AF0)[4] = 0;
        ((s16 *)&D_80126AF0)[5] = -0x1E;
        ((s16 *)&D_80126AF0)[6] = -0x52;
    } else {
        e = k & 0x80FFFFFF;
        ((s16 *)&D_80126AF0)[0] = (s8) * (u8 *)(e + *cur * 3);
        ((s16 *)&D_80126AF0)[1] = (s8) * (u8 *)(e + *cur * 3 + 1);
        ((s16 *)&D_80126AF0)[2] = (s8) * (u8 *)(e + *cur * 3 + 2);
        ((s16 *)&D_80126AF0)[4] = (s8) * (u8 *)(e + *cur * 3 + 3);
        ((s16 *)&D_80126AF0)[5] = (s8) * (u8 *)(e + *cur * 3 + 4);
        ((s16 *)&D_80126AF0)[6] = (s8) * (u8 *)(e + *cur * 3 + 5);
    }
    if ((k & 0xF000000) == 0) {
        (*cur)++;
    }
    func_80156ECC(o, D_801270A8, ((s16 *)&D_80126AF0), ((s16 *)&D_80126AF0) + 4, ((s16 *)&D_80126AF0) + 8);
}
