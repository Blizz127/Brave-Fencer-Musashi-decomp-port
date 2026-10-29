/* Owner re-derivation from retail assembly (clean input set). */
/* Per-object update step: runs the kind handler D_8017F7A4[o->kind], then (for a live object) makes
 * it follow its parent (guest pointer at 0x64): copies the parent's word at 4 and 0xC and its u16 at
 * 0xA plus the object's own offset 0xFC. With the s16 at 0x70 clear the object follows only while
 * the parent's s16 at 0x36 equals the object's s16 at 0xFE; otherwise func_8012C218 is called. */
#include "psx_types.h"

/* Object fields used here; the parent has the same layout. */
typedef struct {
    u16 live;
    u16 kind;
    u32 f4;
    u8 pad8[2];
    u16 fA;
    u32 fC;
    u8 pad10[0x36 - 0x10];
    s16 f36;
    u8 pad38[0x64 - 0x38];
    u32 parent;
    u8 pad68[0x70 - 0x68];
    s16 f70;
    u8 pad72[0xFC - 0x72];
    u16 fFC;
    s16 fFE;
} Obj;

extern u32 D_8017F7A4[]; /* guest handler table: u32 elements, cast at use */
extern void func_8012C218(Obj *o);

s32 func_801424E4(Obj *o)
{
    Obj *t;

    ((void (*)(Obj *))D_8017F7A4[o->kind])(o);
    if (o->live != 0) {
        if (o->f70 != 0) {
            t = (Obj *)o->parent;
            if (t == 0) {
                return;
            }
            o->f4 = t->f4;
            o->fA = t->fA + o->fFC;
            o->fC = t->fC;
        } else {
            if (o->parent != 0 && ((Obj *)o->parent)->f36 == o->fFE) {
                o->f4 = ((Obj *)o->parent)->f4;
                o->fA = ((Obj *)o->parent)->fA + o->fFC;
                o->fC = ((Obj *)o->parent)->fC;
            } else {
                func_8012C218(o);
            }
        }
    }
}
