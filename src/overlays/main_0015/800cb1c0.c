/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* In game mode 8 runs the state handler D_800CBB38[o->state]; otherwise func_800CB474. */
#include "psx_types.h"

typedef struct {
    u16 pad0;
    u16 state;          /* 0x2 */
} Obj;

extern u8 D_80078EC1;
extern u32 D_800CBB38[];
extern void func_800CB474(Obj *o);

void func_800CB1C0(Obj *o)
{
    if (D_80078EC1 == 8) {
        ((void (*)(Obj *))D_800CBB38[o->state])(o);
    } else {
        func_800CB474(o);
    }
}
