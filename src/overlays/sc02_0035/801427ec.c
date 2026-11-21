/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;
} Obj;

extern s16 D_801270C0;
extern s32 (*D_80185918[])(Obj *o);

/* State-machine step: dispatches to the handler for this object's current
 * state (index f2) unless the global phase counter has reached 3. The ABI
 * return type is unresolved. Dispatch propagates the handler result.
 * Skipping dispatch preserves the guest return register. */
s32 func_801427EC(Obj *o) {
    if (D_801270C0 != 3) {
        return D_80185918[o->f2](o);
    }
}
