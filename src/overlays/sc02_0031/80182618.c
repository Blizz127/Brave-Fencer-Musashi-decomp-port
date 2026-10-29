/* Owner re-derivation from retail assembly (clean input set). */
/* When the object's readiness query returns 1: finishes the object, sets its state field (offset 2) to 5, raises event 0x9C and resets the shared message state; returns 1 then, else 0. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 state;
} Obj;

extern s32 func_80178970(Obj *o);
extern void func_80178D18(Obj *o);
extern void func_80029124(s32 a, s32 b);
extern void func_80175414(void);

s32 func_80182618(Obj *o)
{
    s32 r;

    if (func_80178970(o) != 1) {
        r = 0;
    } else {
        func_80178D18(o);
        o->state = 5;
        func_80029124(0x9C, 1);
        func_80175414();
        r = 1;
    }
    return r;
}
