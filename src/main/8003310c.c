/* Owner re-derivation from retail assembly (clean input set). */
/* Finds a free sound slot (returns slot+1); otherwise steals the lowest-priority busy slot if its priority does not exceed `priority`, or returns 0. */
#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 priority;
    u8 rest[0x54 - 4];
} SoundSlot;

typedef struct {
    u8 found;
    s32 index;
    u16 priority;
} SlotChoice;

extern void func_8003324C(u16);
extern SoundSlot D_800A46E8[8];

s32 func_8003310C(s32 priority) {
    SlotChoice best;
    SoundSlot *slot;
    s32 i;

    best.found = 0;
    for (slot = D_800A46E8, i = 0; i < 8; i++, slot++) {
        if (slot->kind == 0) {
            return i + 1;
        }
        if (best.found == 0) {
            best.found = 1;
            best.priority = slot->priority;
            best.index = i;
        } else if (slot->priority < best.priority) {
            best.priority = slot->priority;
            best.index = i;
        }
    }
    if ((u16)priority < best.priority) {
        return 0;
    }
    func_8003324C(best.index);
    return best.index + 1;
}
