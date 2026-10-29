/* Owner re-derivation from retail assembly (clean input set). */
/* Per-object hook step: unless the object type is 0x16 or 0x1A, and its flag word has no bits outside
 * 0x7F000000, asks func_80165658 about the current mode D_80078EC1 and, if bit 7 of the answer is set,
 * calls the handler D_80180FE4[mode & 0x7F] on the object. Then, for types other than 0x16/0x1A, calls
 * func_800CAE0C when the game byte at D_80078E78 + 0x49 is 0x19. */
#include "psx_types.h"

/* Object header: type at 0, flag word at 0x1F8. */
typedef struct {
    u16 type;
    u8 pad[0x1F6];
    u32 flags;
} Obj;

extern u8 D_80078E78[];
extern u8 D_80078EC1;
extern u32 D_80180FE4[]; /* guest handler table: u32 elements, cast at use */
extern s32 func_80165658(Obj *o, s32 mode);
extern void func_800CAE0C(Obj *o);

void func_801654A8(Obj *o)
{
    u8 *g = D_80078E78;

    if (o->type != 0x16 && o->type != 0x1A) {
        if ((o->flags & 0x80FFFFFF) == 0 && (func_80165658(o, D_80078EC1) & 0x80)) {
            ((void (*)(Obj *))D_80180FE4[D_80078EC1 & 0x7F])(o);
        }
    }
    if (o->type != 0x16 && o->type != 0x1A) {
        if (g[0x49] == 0x19) {
            func_800CAE0C(o);
        }
    }
}
