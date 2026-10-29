/* Owner re-derivation from retail assembly (clean input set). */
/* Resets object dispatch state: points the handler-table field back at the default table
 * D_801847FC, clears three tracking fields, drops flag bits 0x0200/0x0400 from the state word,
 * then runs func_8012AAAC(o). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x72];
    u16 flags;   /* 0x72 */
    u8 pad74[0x90 - 0x74];
    u32 table;   /* 0x90: guest pointer */
    s32 f94;     /* 0x94 */
    u8 pad98[2];
    s16 f9A;     /* 0x9A */
    s32 f9C;     /* 0x9C */
} Obj;

extern u8 D_801847FC[];
extern void func_8012AAAC(Obj *o);

void func_8012ACA0(Obj *o)
{
    u16 flags;

    flags = o->flags;
    o->table = (u32)D_801847FC;
    o->f94 = 0;
    o->f9C = 0;
    o->f9A = 0;
    o->flags = flags & 0xF9FF;
    func_8012AAAC(o);
}
