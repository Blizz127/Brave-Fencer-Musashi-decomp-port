/* Owner re-derivation from retail assembly (clean input set). */
/* Searches the 8 slots of D_800A46E8 for an active type-5 slot whose id (low 16 bits) and optional sub-id (high 16 bits) match; returns slot+1 or 0. */
#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 pad2;
    u16 id;
    u16 sub_id;
    u8 rest[0x54 - 8];
} Slot800A46E8;

extern Slot800A46E8 D_800A46E8[8];

s32 func_800348A8(u32 value) {
    Slot800A46E8 *slot;
    s32 i;
    u16 id = value;
    u16 sub_id = value >> 16;

    slot = D_800A46E8;
    for (i = 0; i < 8; i++, slot++) {
        if (slot->kind == 5 && slot->id == id && (sub_id == 0 || slot->sub_id == sub_id)) {
            return ++i;
        }
    }
    return 0;
}
