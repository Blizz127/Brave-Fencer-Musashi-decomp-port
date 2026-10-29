/* func_8016C74C - owner re-derivation, clean room (nmO).
 * func_80047E98 reads a0 (in) and stores three halfwords through a1 (out);
 * retail relies on a1 still holding v from entry. The call is written with
 * both args explicit: (v, v). CSE deletes the redundant `a1 = v` (a1 still
 * holds v), and the explicit a1 use also keeps the a3 (sh2) copy in the
 * prologue as retail. callers read v0: none-direct (void). */
#include "psx_types.h"
typedef struct { s16 x, y, z; } SV8016C74C;
typedef struct { u8 pad[0x10]; s32 x, y, z; } O8016C74C;
typedef struct { u8 pad[0x10]; u16 xl, xh, yl, yh, zl, zh; } OH8016C74C;
extern s32 func_80047E98(SV8016C74C *in, SV8016C74C *out);

void func_8016C74C(O8016C74C *o, SV8016C74C *v, s16 sh, s16 sh2, s32 limit)
{
    o->x += v->x << sh;
    o->y += v->y << sh;
    o->z += v->z << sh;
    v->x = ((OH8016C74C *)o)->xh;
    v->y = ((OH8016C74C *)o)->yh;
    v->z = ((OH8016C74C *)o)->zh;
    if (limit < func_80047E98(v, v)) {
        o->x = v->x << sh2;
        o->y = v->y << sh2;
        o->z = v->z << sh2;
    }
}
