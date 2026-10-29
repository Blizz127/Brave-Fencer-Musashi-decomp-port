/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
typedef struct {
    u16 x;
    u16 y;
    u16 z;
} Vec8014ACE8;

extern s32 func_8014AD30(char *o, Vec8014ACE8 *v, s32 b, s32 c);

/* Returns retail's leftover v0: func_8014AD30's result from the final call. */
s32 func_8014ACE8(char *o, s32 b, s32 c)
{
    Vec8014ACE8 v;

    v.x = *(u16 *)(o + 6);
    v.y = *(u16 *)(o + 0xA) - 0x20;
    v.z = *(u16 *)(o + 0xE);
    return func_8014AD30(o, &v, b, c);
}
