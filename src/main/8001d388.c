/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches an active type-2 object through the handler table indexed by its subtype. */
#include "psx_types.h"

typedef struct {
    u16 type;
    u16 subtype;
    s32 state;
    u8 pad[0x18];
    s32 enabled;
} Obj8001D388;

extern u32 D_8006351C[];

void func_8001D388(Obj8001D388 *obj)
{
    if (obj->state >= 0 && obj->enabled != 0 && obj->type == 2 && obj->subtype < 11) {
        ((void (*)(Obj8001D388 *))D_8006351C[obj->subtype])(obj);
    }
}
