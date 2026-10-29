/* Owner re-derivation from retail assembly (clean input set). */
/* func_801483E8 [801483E8,801484B0) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { s16 x; s16 y; s16 z; u8 k; u8 pad; } B;

/* callers read v0: no (all direct callers ignore it; retail sets none). */
extern void func_80015978(u8 *src, u16 *dst);
extern s32 func_80133784(s32 a, s16 *p, s16 *q);

void func_801483E8(u8 *o) {
    u16 a[4];
    B b;

    func_80015978(o + 4, a);
    b.x = a[0];
    b.y = a[1] + 8;
    b.z = a[2];
    if (func_80133784(1, (s16 *)a, (s16 *)&b) == 0x2000 && *(s16 *)(o + 6) == b.x && *(s16 *)(o + 0xA) == b.y
        && *(s16 *)(o + 0xE) == b.z && b.k == 0) {
        func_80015978(o + 4, (u16 *)(o + 0xA0));
    }
}
