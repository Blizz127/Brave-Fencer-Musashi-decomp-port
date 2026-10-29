/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u16 id;
    /* 0x002 */ u8 pad02[0x6E];
    /* 0x070 */ s16 f70;
    u8 pad_end[0x10C - 0x70 - 2];
} Slot; /* 0x10C bytes */

extern u8 D_801202A0[];

/* Slot of the 96-entry table whose id and field 0x70 both match, or NULL.
 * The first argument is unused. */
Slot *func_8014C118(s32 unused, s32 id, s16 v)
{
    u32 i;
    Slot *p;

    for (i = 0; i < 96; i++) {
        p = &((Slot *)&D_801202A0)[i];
        if (p->id == id && p->f70 == v) {
            return p;
        }
    }
    return 0;
}
