/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements D_80078EC8 if nonzero, unless masked bits at +0x1F8 are set. */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x1F8];
    u32 flags1F8;
} Obj80165694;

extern s32 D_80078EC8;

void func_80165694(Obj80165694 *obj)
{
    if ((obj->flags1F8 & 0x80FFFFFF) == 0) {
        if (D_80078EC8 != 0) {
            D_80078EC8--;
        }
    }
}
