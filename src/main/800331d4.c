/* Owner re-derivation from retail assembly (clean input set). */
/* Searches the 8 sound slots for a kind-1 slot whose id (low 16 bits) and optional sub-id (high 16 bits) match; returns slot+1 or 0. */
#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 pad2;
    u16 id;
    u16 sub_id;
    u8 rest[0x54 - 8];
} SoundSlot;

extern SoundSlot D_800A46E8[8];

s32 func_800331D4(u32 value) {
    SoundSlot *slot;
    s32 i;
    u16 id = value;
    u16 sub_id = value >> 16;

    slot = D_800A46E8;
    i = 0;
    do {
        if ((slot->kind & 0x3F) == 1 && slot->id == id && (sub_id == 0 || sub_id == slot->sub_id)) {
            return ++i;
        }
        i++;
        slot++;
    } while (i < 8);
    return 0;
}
