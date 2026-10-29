/* Owner re-derivation from retail assembly (clean input set). */
/* Stops the active slots playing a sound id (low half; high half = optional sub id), resolving redirect entries first. */
#include "psx_types.h"

typedef struct SoundEntry {
    u8 kind;
    u8 pad[3];
} SoundEntry;

typedef struct Slot {
    u16 state;
    u8 pad2[2];
    u16 id;
    u16 sub;
    u8 pad8[0xE];
    u8 stop;
    u8 pad17[0x54 - 0x17];
} Slot;

typedef struct SlotTable {
    Slot slots[24];
    u8 pad7E0[0x4F];
    u8 busy;
} SlotTable;

extern SoundEntry D_8006451C[];
extern SlotTable D_800A46E8;

extern s32 func_8002F4E4(SoundEntry *e);

void func_8003491C(u32 arg)
{
    s16 cur;
    s16 sub;
    SoundEntry *e;
    u8 saved;
    s32 kind;
    s32 i;
    Slot *s;

    cur = arg;
    e = &D_8006451C[(s16)arg];
    sub = arg >> 16;
    if ((e->kind & 0x7F) == 6) {
        cur = func_8002F4E4(e);
        if (cur == 0) {
            return;
        }
        e = &D_8006451C[cur];
    }
    if (cur < 0x100) {
        return;
    }
    saved = D_800A46E8.busy;
    D_800A46E8.busy = 1;
    kind = e->kind & 0x7F;
    if (kind != 1 && kind == 5) {
        s = D_800A46E8.slots;
        for (i = 0; i < 8; i++, s++) {
            if (s->state == 5 && s->id == (s16)arg && (sub == 0 || sub == s->sub)) {
                s->stop = 1;
            }
        }
    }
    D_800A46E8.busy = saved;
}
