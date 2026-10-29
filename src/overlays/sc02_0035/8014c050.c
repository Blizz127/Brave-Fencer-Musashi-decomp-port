/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u16 id;
    /* 0x02 */ u8 pad02[0x56];
} Slot58;

extern u8 D_8011D030[];

/* Slot of the 30-entry table whose id matches, or NULL. */
Slot58 *func_8014C050(s32 unused, s32 id)
{
    u32 i = 0;
    Slot58 *p = ((Slot58 *)&D_8011D030);

    for (; i < 30; i++, p++) {
        if (p->id == id) {
            return p;
        }
    }
    return 0;
}
