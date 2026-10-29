/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches an active type-1 object through its subtype handler table and bumps a counter. */
#include "psx_types.h"

typedef struct {
    u16 type;
    u16 subtype;
    s32 state;
} Obj8001FB8C;

extern u32 D_80063500[];

void func_8001FB8C(Obj8001FB8C *obj, s32 *count)
{
    if (obj->state >= 0 && obj->type == 1 && obj->subtype < 9) {
        ((void (*)(Obj8001FB8C *, s32 *))D_80063500[obj->subtype])(obj, count);
        *count += 1;
    }
}
