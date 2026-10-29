/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x16C];
    u16 f16C;
} Obj80150B28;

extern u32 D_800AE6B0;
extern u32 D_8017FFFC[]; /* guest handler table */

void func_80150B28(Obj80150B28 *o) {
    u32 id;
    s32 k;

    if ((D_800AE6B0 & 0x80FFFFFF) == 0) {
        id = o->f16C;
        if (id != 0 && id < 0x26) {
            k = id - 1;
            ((void (*)(Obj80150B28 *))D_8017FFFC[k])(o);
        }
    }
    o->f16C = 0;
}
