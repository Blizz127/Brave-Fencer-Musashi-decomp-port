/* Owner re-derivation from retail assembly (clean input set). */
/* Sets flag bit 3 on the object, then dispatches to its state handler. The sole in-region caller
 * (func_80165580) never reads the handler's result, so this function and its handler call are void. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x44];
    u32 flags;   /* 0x44 */
    u8 pad48[0x197 - 0x48];
    u8 state;    /* 0x197 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_80187B44[]; /* guest pointers: per-state handler table */

void func_80165B28(Obj *o)
{
    o->flags |= 8;
    ((Handler)D_80187B44[o->state])(o);
}
