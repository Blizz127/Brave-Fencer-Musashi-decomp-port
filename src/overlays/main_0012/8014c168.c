/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u16 id;
    /* 0x002 */ u8 pad02[0x10A];
} Slot;

extern u8 D_801202A0[];

/* Slot of the 96-entry table whose id matches, or NULL. */
Slot *func_8014C168(s32 unused, s32 id)
{
    u32 i = 0;
    Slot *p = ((Slot *)&D_801202A0);

    for (; i < 96; i++, p++) {
        if (p->id == id) {
            return p;
        }
    }
    return 0;
}
