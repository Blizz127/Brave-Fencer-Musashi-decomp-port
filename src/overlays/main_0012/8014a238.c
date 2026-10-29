/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the first live pool entry (D_801202A0, 0x60 x 0x10C) with a nonzero +0xAA field that is
 * within range 0x30 of o and passes func_8014C2B0 at 0x200, or NULL. */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 pad02[0xA8];
    s16 field_AA; /* 0xAA */
    u8 padAC[0x60];
} Ent8014A238; /* 0x10C bytes */

extern u8 D_801202A0[];
extern s32 func_8014C278(void *self, Ent8014A238 *ent, s32 range);
extern s32 func_8014C2B0(void *self, Ent8014A238 *ent, u16 limit);

Ent8014A238 *func_8014A238(void *o)
{
    s32 i;
    Ent8014A238 *e;

    i = 0;
    do {
        e = (Ent8014A238 *)((u8 *)((Ent8014A238 *)&D_801202A0) + i * 0x10C);
        if (e->id != 0 && e->field_AA != 0 && func_8014C278(o, e, 0x30) != 0
            && func_8014C2B0(o, e, 0x200) != 0) {
            return e;
        }
        i++;
    } while ((u32)i < 0x60);
    return 0;
}
