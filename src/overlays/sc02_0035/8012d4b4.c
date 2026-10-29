/* Owner re-derivation from retail assembly (clean input set). */
/* Shares its tail logic with the sibling func_8012D3B4 draft (same overlay region,
 * immediately preceding address range): this version first derives the two buffer
 * values via func_8012F214 instead of taking them as direct params. */
#include "psx_types.h"

typedef struct { u32 f0; u8 p4[16]; } T;
typedef struct { u32 f0; u32 f4; } R;

extern void func_8012F214(void *o, void *v, void *out);
extern void *func_80010A08(s32 size);
extern void func_80058E9C(void *p);
extern void func_8004914C(void *p);
extern void func_800491AC(void *p);
extern s32 func_8004955C(s32 a, void *b, void *c, void *d);
extern void func_80058C6C(u32 p, void *q);
extern u8 D_800AF648;
extern u16 D_800B9A02;
extern u8 D_800A651C[];

void func_8012D4B4(void *o, void *v1, void *v2, u32 c) {
    u8 bufA[8];
    u8 bufB[8];
    R *p;
    s32 n;
    s32 x;
    s32 y;

    func_8012F214(o, v1, bufA);
    func_8012F214(o, v2, bufB);
    p = func_80010A08(0x10);
    p->f4 = c;
    func_80058E9C(p);
    func_8004914C(&D_800AF648);
    func_800491AC(&D_800AF648);
    n = func_8004955C((s32)bufA, (u8 *)p + 8, &x, &y);
    if (n > 0 && y >= 0) {
        if (func_8004955C((s32)bufB, (u8 *)p + 0xC, &x, &y) > 0 && y >= 0) {
            func_80058C6C(((T *)&D_800A651C)[D_800B9A02].f0 + n * 4, p);
        }
    }
}
