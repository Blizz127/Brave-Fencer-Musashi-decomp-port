/* Owner re-derivation from retail assembly (clean input set). */
/* Copies six s16 globals (D_80182698.. D_801826A4) into six s32 fields at offsets 0x3C..0x50 of the object; the stores are written through raw offsets to keep each global load next to its store. */
#include "psx_types.h"

extern s16 D_80182698;
extern s16 D_8018269A;
extern s16 D_8018269C;
extern s16 D_801826A0;
extern s16 D_801826A2;
extern s16 D_801826A4;

/* Returns retail's leftover v0: D_801826A4, the last halfword loaded (sign-extended). */
s32 func_8017D428(u8 *o)
{
    s32 r;

    *(s32 *)(o + 0x3C) = D_80182698;
    *(s32 *)(o + 0x40) = D_8018269A;
    *(s32 *)(o + 0x44) = D_8018269C;
    *(s32 *)(o + 0x48) = D_801826A0;
    *(s32 *)(o + 0x4C) = D_801826A2;
    r = D_801826A4;
    *(s32 *)(o + 0x50) = r;
    return r;
}
