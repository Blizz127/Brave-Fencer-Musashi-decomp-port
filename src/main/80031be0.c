/* Owner re-derivation from retail assembly (clean input set). */
/* Unless slot updates are paused, publishes each of the 8 slots' two status words and runs its per-kind tick (kind 1: count down its timer; kind 5: func_8003388C). */
#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 pad2;
    u16 status_a;
    u16 status_b;
    u16 timer;
    u8 rest[0x54 - 0xA];
} Slot;

typedef struct {
    Slot slots[8];
    u8 pad2A0[0x82F - 0x2A0];
    u8 paused;
} SlotState;

extern void func_8003388C(Slot *, s32);
extern u16 D_800A2BB0[];
extern SlotState D_800A46E8;
extern u16 D_800BA108[];

/* Returns retail's leftover v0: the nonzero paused byte when updates are paused, else the closing loop flag (i < 8, 0 at exit). */
s32 func_80031BE0(void) {
    Slot *slot;
    s32 i;
    s32 r;

    r = D_800A46E8.paused;
    if (r == 0) {
        slot = D_800A46E8.slots;
        i = 0;
        do {
            D_800A2BB0[i] = slot->status_a;
            D_800BA108[i] = slot->status_b;
            switch (slot->kind & 0x3F) {
            case 0:
                break;
            case 1:
                if (slot->timer != 0) {
                    slot->timer--;
                }
                break;
            case 5:
                func_8003388C(slot, i);
                break;
            }
            i++;
            slot++;
            r = i < 8;
        } while (r != 0);
    }
    return r;
}
