/* Owner re-derivation from retail assembly (clean input set). */
/* Releases the object's focus: unless it has a sub-record (pointer at +0x40) with a nonzero +0x18 and a first half-word other than 4, and when the current focus D_80127524 is this object, takes the queued object D_80127528 as the new focus (clearing the queue), marks its +0x1A half-word 1 and runs func_80138C30 on it; then sets the state half-word (offset 4) to 0xE when bit 0x8000 of the flag word at +8 is set, else 0xD. */
#include "psx_types.h"

typedef struct {
    s16 h0;
    u8 pad2[0x18];
    s16 h1A;
} Sub;

typedef struct {
    u8 pad0[4];
    s16 h4;
    u8 pad6[2];
    u32 w8;
    u8 padC[0xC];
    u16 h18;
    u8 pad1A[0x26];
    Sub *p40;
} Obj;

extern Obj *D_80127524;
typedef struct {
    Obj *p;
} Cur;
extern Obj *D_80127528;
extern void func_80138C30(Obj *o);

/* Returns retail's leftover v0: the compare constant 4 when the guard skips, else the state 0xE / 0xD stored to +4. */
s32 func_8013A2BC(Obj *o)
{
    Obj *t;
    s32 r;

    if (o->p40 == 0 || o->h18 == 0 || o->p40->h0 == 4) {
        if (((Cur *)&D_80127524)->p == o) {
            t = D_80127528;
            D_80127528 = 0;
            ((Cur *)&D_80127524)->p = t;
            if (t != 0) {
                ((Sub *)D_80127524)->h1A = 1;
                func_80138C30(((Cur *)&D_80127524)->p);
            }
        }
        r = o->w8 & 0x8000;
        if (r != 0) {
            r = 0xE;
        } else {
            r = 0xD;
        }
        o->h4 = r;
    } else {
        r = 4;
    }
    return r;
}
