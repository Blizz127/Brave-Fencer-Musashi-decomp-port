/* Owner re-derivation from retail assembly (clean input set). */
/* Per-type handler step. If o->f44 bit 0x10 is set, dispatches the per-type handler
 * table, then runs func_80147078(o, 0) and func_80159B70(o), and returns. Otherwise,
 * when o->f1AC is nonzero it is advanced by -0xAA8 (always stored back); if that went
 * nonnegative, runs func_80161208(o) and returns. When o->f1AC is zero, calls
 * func_80148800(o) and checks its result & 0xF0; if zero, runs func_80161208(o) and
 * returns. Otherwise (f1AC went negative, or the 0x80148800 check was nonzero) falls
 * into the same dispatch+80147078+80159B70 sequence as the first branch. Reached only
 * via a jalr sub-table (D_8018730C, type-indexed) dispatched itself from the outer
 * per-state table D_80180E18 through func_80160B34; neither jalr site reads v0. */
#include "psx_types.h"

typedef struct Obj {
    u16 type;      /* 0x000 */
    u8 pad002[0x44 - 2];
    u32 f44;       /* 0x44 */
    u8 pad48[0x1AC - 0x48];
    s32 f1AC;      /* 0x1AC */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[]; /* guest pointers: per-type handler table */

extern s32 func_80148800(Obj *o);
extern void func_80147078(Obj *o, s32 v);
extern void func_80159B70(Obj *o);
extern void func_80161208(Obj *o);

void func_80160D10(Obj *o)
{
    s32 v0;
    s32 v1;

    if (o->f44 & 0x10) {
        ((Handler)D_8018730C[o->type])(o);
        func_80147078(o, 0);
        func_80159B70(o);
        return;
    }

    v1 = o->f1AC;
    if (v1 == 0) {
        v0 = func_80148800(o);
        if ((v0 & 0xF0) == 0) {
            goto do_161208;
        }
    } else {
        v0 = v1 + 0xFFFF5558;
        o->f1AC = v0;
        if (v0 >= 0) {
            goto do_161208;
        }
    }

    ((Handler)D_8018730C[o->type])(o);
    func_80147078(o, 0);
    func_80159B70(o);
    return;

do_161208:
    func_80161208(o);
}
