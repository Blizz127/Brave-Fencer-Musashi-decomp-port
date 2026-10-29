/* Owner re-derivation from retail assembly (clean input set). */
/* Starts the approach state: copies the camera target, derives per-axis step velocities (1/30 of the offset, 16.16) and requests the handler calls. */
#include "psx_types.h"

typedef struct {
    u8 p0[4];
    u8 f4[2];
    s16 s6;
    u8 p8[2];
    s16 sA;
    u8 pC[2];
    s16 sE;
    s32 f10;
    s32 f14;
    s32 f18;
    s32 f1C;
    u8 *f20;
    u8 p24[0x2C];
    s16 s50;
    s16 s52;
    s16 s54;
    u8 p56[2];
    u32 f58;
    s16 s5C;
    u8 p5E[0x98 - 0x5E];
    s16 s98;
    u8 p9A[0xAC - 0x9A];
    s16 sAC;
    u8 pAE[6];
    u32 fB4;
    u8 pB8[9];
    u8 fC1;
    u8 pC2[2];
    u32 fC4;
} O;

extern u32 D_801151D4;
extern s32 func_8012B744(s16 *a, s16 *b);
extern void func_8012C588(s32 id, void *o);
extern s32 func_80131A34(void *o, s32 a);
extern void func_80131CA8(void *o, s32 a);

void func_8012F968(O *o) {
    u8 *c = (u8 *)D_801151D4;
    s16 v[3];
    u8 *p;
    o->fC1 = 0x12;
    if (o->fB4 & 0x10000) {
        v[0] = *(s32 *)(c + 0x5C);
        v[1] = *(s32 *)(c + 0x60);
        v[2] = *(s32 *)(c + 0x64);
        o->s50 = o->s52 = o->s54 = o->s5C = 0;
        *(s16 *)(o->f20 + 0x10) = *(s16 *)(o->f20 + 0x14) = 0;
        *(s16 *)(o->f20 + 0x12) = func_8012B744((s16 *)o->f4, v);
        o->f1C = 0x2D;
        if (o->f58 != 0) {
            p = (u8 *)((o->f58 & 0xFFFFFFF) | 0x80000000);
            o->sAC = (*(s16 *)(p + 8) + *(s16 *)(p + 0xA)) >> 1;
        } else {
            o->sAC = 0;
        }
        o->f10 = ((*(s32 *)(c + 0x5C) - o->s6) << 16) / 30;
        o->f14 = ((*(s32 *)(c + 0x60) - o->sA - o->sAC) << 16) / 30;
        o->f18 = ((*(s32 *)(c + 0x64) - o->sE) << 16) / 30;
        o->s98 = 0;
        if (func_80131A34(o, 0x24) == 0) {
            o->fC4 &= ~4;
            if (func_80131A34(o, 0x20) != 0) {
                o->fC4 |= 4;
            } else {
                o->s98 = 0;
            }
        }
        func_8012C588(0x289, o);
    }
    func_80131CA8(o, 0x35);
}
