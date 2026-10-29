/* Owner re-derivation from retail assembly (clean input set). */
/* Stops a sound by id (resolving redirect entries): stream/sequence/one-shot kinds go to their stop handlers, kind 17 fades its bus; slot updates are paused meanwhile. */
#include "psx_types.h"

typedef struct {
    u8 kind;
    u8 pad1[2];
    u8 bus;
} SoundEntry;

typedef struct {
    u8 slots[0x82F];
    u8 paused;
} SlotState;

extern s16 func_8002F4E4(SoundEntry *);
extern void func_8002F714(u16, s32);
extern void func_80033398(s32);
extern void func_800346D0(s32);
extern SoundEntry D_8006451C[];
extern SlotState D_800A46E8;

void func_8002EED8(s32 id) {
    SoundEntry *entry;
    s16 redirect;
    u8 paused;

    if ((s16)id < 0x100) {
        return;
    }
    entry = &D_8006451C[(s16)id];
    if ((entry->kind & 0x7F) == 6) {
        redirect = func_8002F4E4(entry);
        if (redirect == 0) {
            return;
        }
        entry = &D_8006451C[redirect];
    }
    paused = D_800A46E8.paused;
    D_800A46E8.paused = 1;
    switch (entry->kind & 0x3F) {
    case 5:
        func_800346D0(id);
        break;
    case 1:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 18:
        func_80033398(id);
        break;
    case 17:
        func_8002F714(entry->bus, 0x1000);
        break;
    }
    D_800A46E8.paused = paused;
}
