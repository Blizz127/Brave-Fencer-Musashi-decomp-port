/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatch-table handler (D_8017FD00[0] in main_0012, paired with func_80143D28 at [1]).
 * Allocates a sub-object (func_8012C1B8) into o->f20; on failure, cleans up via
 * func_8012CAE4 and returns. On success, inits it (func_8001CA1C) against the
 * owner table D_80185E10, copies a timer field from o->f64 into o->fFC, nudges
 * o->fA down by a fixed or state-derived amount, re-inits via func_8012A828
 * against D_80185E1C, sets state=1, computes a /3 scaled value into the new
 * sub-object, and resets o->f1C to a frame count. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 pad2[0x34];
    u16 f36;
    u8 pad38[0x20];
    u32 f58;
} State;

typedef struct {
    u8 pad0[2];
    s16 f2;
    u8 pad4[6];
    u16 fA;
    u8 padC[0x10];
    s32 f1C;
    void *f20;
    u8 pad24[0x40];
    State *f64;
    u8 pad68[8];
    s16 f70;
    u8 pad72[0x8A];
    s16 fFC;
} Obj;

extern void *func_8012C1B8(void);
extern void func_8012CAE4(u8 *o);
extern void func_8001CA1C(void *p, s32 a1);
extern s32 func_8012A828(Obj *o, u32 a);
extern u8 D_80185E10[];
extern u8 D_80185E1C[];

#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_8014477C(Obj *o)
{
    u32 raw;

    o->f20 = func_8012C1B8();
    if (o->f20 == 0) {
        func_8012CAE4((u8 *)o);
        RETURN_NO_VALUE;
    }

    func_8001CA1C(o->f20, (s32)D_80185E10);

    if (o->f64->f0 != 0) {
        o->fFC = o->f64->f36;
        raw = o->f64->f58;
        if (raw != 0) {
            s16 adj = o->fA;
            adj -= 0x30;
            o->fA = *(u16 *)(((raw & 0x0FFFFFFF) | 0x80000000) + 8) + adj;
        } else {
            o->fA -= 0x60;
        }
    }

    func_8012A828(o, (u32)D_80185E1C);
    o->f2 = 1;
    *(s16 *)((u8 *)o->f20 + 0x12) = (s16)(((s32)(o->f70 << 12)) / 3);
    o->f1C = 0x78;
    RETURN_NO_VALUE;
}
