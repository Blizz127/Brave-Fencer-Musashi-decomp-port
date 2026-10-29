/* Owner re-derivation from retail assembly (clean input set). */
/* Counts down the word at +0x1C; returns 1 when it is already zero or reaches zero. */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x1C];
    s32 timer1C;
} Obj8012BEE8;

s32 func_8012BEE8(Obj8012BEE8 *obj)
{
    s32 t = obj->timer1C;

    if (t != 0) {
        t--;
        obj->timer1C = t;
        return t == 0;
    }
    return 1;
}
