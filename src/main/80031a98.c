/* Owner re-derivation from retail assembly (clean input set). */
/* Walks the 8 active slots and releases (func_80030D80 mode 1) each one whose table entry is negative, or whose id has bit 0x80 while D_800A46B0 is set. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xE];
    u16 id;
    u8 pad10[0x36 - 0x10];
    u8 use_alt;
    u8 pad37[0x4E - 0x37];
    u8 active;
    u8 pad4F[0x54 - 0x4F];
} Slot800A4988;

typedef struct {
    s16 normal;
    s16 alt;
} Entry800C5328;

extern void func_80030D80(Slot800A4988 *, s32);
extern u8 D_800A46B0;
extern Slot800A4988 D_800A4988[8];
extern Entry800C5328 D_800C5328[];

void func_80031A98(void) {
    Slot800A4988 *slot;
    s32 i;
    s32 value;

    slot = D_800A4988;
    i = 0;
    do {
        if (slot->active != 0) {
            if (slot->use_alt != 0) {
                value = D_800C5328[slot->id].alt;
            } else if (slot->id & 0x80) {
                if (D_800A46B0 != 0) {
                    func_80030D80(slot, 1);
                }
                goto next;
            } else {
                value = D_800C5328[slot->id].normal;
            }
            if (value < 0) {
                func_80030D80(slot, 1);
            }
        }
    next:
        i++;
        slot++;
    } while (i < 8);
}
