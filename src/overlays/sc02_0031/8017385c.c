/* Owner re-derivation from retail assembly (clean input set). */
/* Sets flag bit 0x80 on o->fAC/fAA when D_80126B30==0 or anything other than 1, else bit 0x10
 * when it is exactly 1; then func_80171A1C(o); func_80174650(o). void abi: sampled caller's next
 * instr is a plain epilogue restore. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xAA];
    u16 fAA; /* 0xAA */
    u16 fAC; /* 0xAC */
} Obj;

extern s16 D_80126B30;
extern void func_80171A1C(void *o);
extern void func_80174650();

void func_8017385C(Obj *o)
{
    s16 v1 = D_80126B30;

    if (v1 == 0) {
        goto use80;
    }
    if (v1 == 1) {
        goto use10;
    }
use80:
    o->fAC = o->fAC | 0x80;
    o->fAA = o->fAA | 0x80;
    goto store;
use10:
    o->fAC = o->fAC | 0x10;
    o->fAA = o->fAA | 0x10;
store:
    func_80171A1C(o);
    func_80174650(o);
}
