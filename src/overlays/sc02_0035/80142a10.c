/* Owner re-derivation from retail assembly (clean input set). */
/* Unless global mode D_801270C0==3, dispatches to the state handler table entry for o->f2 and,
 * if the handler leaves o->fFC nonzero, follows up with func_80142A80(o). No caller found in any
 * available region/overlay for this function; body never sets v0, so it is void. */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;   /* 0x2 */
    u8 padC[0xFC - 0x4];
    s16 fFC;  /* 0xFC */
} Obj;

typedef void (*Handler)(Obj *);

extern s16 D_801270C0;
extern u32 D_80185BCC[]; /* guest pointers: per-f2 handler table */
extern void func_80142A80(Obj *o);

void func_80142A10(Obj *o)
{
    if (D_801270C0 != 3) {
        ((Handler)D_80185BCC[o->f2])(o);
        if (o->fFC != 0) {
            func_80142A80(o);
        }
    }
}
