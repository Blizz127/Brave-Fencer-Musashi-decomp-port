/* Owner re-derivation from retail assembly (clean input set). */
/* Counts how many entries of the 30-slot D_8011D030 pool carry the given id and, below the limit, forwards the eight arguments to func_80146A6C. */
#include "psx_types.h"
typedef struct { u16 id; u8 pad[0x56]; } E;
extern u8 D_8011D030[];
extern s32 func_80146A6C(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g);
s32 func_801469C8(u16 a, s32 b, s16 c, s16 d, s16 e, s32 f, s32 g, u32 lim) {
    u32 n;
    E *p;
    E *end;
    n = 0;
    p = ((E *)&D_8011D030);
    end = p + 30;
    for (; p < end; p++) {
        if (p->id == a) {
            n++;
        }
    }
    if (n < lim) {
        return func_80146A6C(a, b, c, d, e, f, g);
    }
    return 0;
}
