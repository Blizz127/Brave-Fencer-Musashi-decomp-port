/* Owner re-derivation from retail assembly (clean input set). */
/* Finds the first free slot (leading u16 == 0) of the 0x60-entry pool D_801202A0, or NULL. */
#include "psx_types.h"

typedef struct {
    u16 used;
    u8 pad2[0x10A];
} Slot8013E410; /* 0x10C bytes */

extern Slot8013E410 D_801202A0[];

Slot8013E410 *func_8013E410(void)
{
    Slot8013E410 *slot;
    s32 i;

    slot = D_801202A0;
    for (i = 0; i < 0x60; i++, slot++) {
        if (slot->used == 0) {
            return slot;
        }
    }
    return 0;
}
