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
 * here is unresolved (no caller found in this worktree); kept s32 and the
 * dispatched handler's result is propagated, matching the retail bytes
 * either way since the skip path never touches v0. */
s32 func_801427EC(Obj *o) {
    if (D_801270C0 != 3) {
        return D_80185918[o->f2](o);
    }
}
