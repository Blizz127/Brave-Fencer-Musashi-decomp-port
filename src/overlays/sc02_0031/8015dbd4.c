/* Owner re-derivation from retail assembly (clean input set). */
/* Object step: optionally runs func_8015DD74 when flag bit 0x4000 is set. If the state word is exactly
 * 0x8000, dispatches the per-type handler table, then runs func_80147078(o, 0) and func_80159B70(o).
 * Otherwise, when the 0x178 counter is non-zero runs func_8015DCB8(o), then always runs func_80161208(o).
 * No caller reads a result, so this and the handler call are void. */
#include "psx_types.h"

typedef struct {
    u16 type;      /* 0x000 */
    u8 pad002[0xB8 - 2];
    u16 state;     /* 0x0B8 */
    u8 pad0BA[0x178 - 0xBA];
    s32 counter;   /* 0x178 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[]; /* guest pointers: per-type handler table */

extern void func_8015DD74(void);
extern void func_80147078(Obj *o, s32 v);
extern void func_80159B70(Obj *o);
extern void func_8015DCB8(Obj *o);
extern void func_80161208(Obj *o);

void func_8015DBD4(Obj *o)
{
    if (o->state & 0x4000) {
        func_8015DD74();
    }
    if (o->state == 0x8000) {
        ((Handler)D_8018730C[o->type])(o);
        func_80147078(o, 0);
        func_80159B70(o);
    } else {
        if (o->counter != 0) {
            func_8015DCB8(o);
        }
        func_80161208(o);
    }
}
