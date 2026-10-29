/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Counts calls at +0x1C: on the first, halves the level and redraws via func_800CB988; after that, func_80146C3C. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1C];
    s32 count;          /* 0x1C */
    u8 pad20[0x30 - 0x20];
    u32 level;          /* 0x30 */
} Obj;

extern void func_800CB988(Obj *o);
extern void func_80146C3C(void *o);

void func_800CB930(Obj *o)
{
    if (++o->count < 2) {
        o->level >>= 1;
        func_800CB988(o);
    } else {
        func_80146C3C(o);
    }
}
