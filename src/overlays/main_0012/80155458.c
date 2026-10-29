/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the current state handler of an object: state byte (0x4E) 0 means idle, otherwise the handler
 * D_801801EC[state - 1] is called on the object, and a zero result clears the state. Returns the
 * (possibly cleared) state byte. */
#include "psx_types.h"

/* Object header; only the state byte is used here. */
typedef struct {
    u8 pad[0x4E];
    u8 state;
} Obj;

extern u32 D_801801EC[]; /* guest handler table: u32 elements, cast at use */

int func_80155458(Obj *o)
{
    if (o->state != 0) {
        s32 k = o->state - 1;

        if (((s32 (*)(Obj *))D_801801EC[k])(o) == 0) {
            o->state = 0;
        }
    }
    return o->state;
}
