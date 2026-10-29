/* Owner re-derivation from retail assembly (clean input set). */
/* If o->0x98(s16)==0: sets o->2=1, then func_8012A828(o, &D_801AB818) when o->0xFE(s16)==1,
 * else func_8012A828(o, &D_801AB5F8); either way clears o->0xFE afterward. */
#include "psx_types.h"

extern void func_8012A828(u8 *o, void *a1);
extern s32 D_801AB818;
extern s32 D_801AB5F8;

void func_80184A2C(u8 *o)
{
    if (*(s16 *)(o + 0x98) == 0) {
        *(s16 *)(o + 2) = 1;
        if (*(s16 *)(o + 0xFE) == 1) {
            func_8012A828(o, &D_801AB818);
        } else {
            func_8012A828(o, &D_801AB5F8);
        }
        *(s16 *)(o + 0xFE) = 0;
    }
}
