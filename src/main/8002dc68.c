/* Owner re-derivation from retail assembly (clean input set). */
/* Plays a sound by id (resolving redirect entries) with slot updates paused: dispatches on the entry kind to its handler and returns the handler's result. */
#include "psx_types.h"

typedef struct {
    u8 kind;
    u8 pad1[2];
    u8 bus;
} SoundEntry;

extern s32 func_80032048(s32, SoundEntry *, u16);
extern s32 func_800322A8(s32, SoundEntry *, u16);
extern s32 func_800324A4(s32, SoundEntry *, u16, s32);
extern s32 func_80032774(s32, SoundEntry *, u16);
extern s32 func_80034314(u32, void *, u32);
extern void func_8002F248(SoundEntry *, s32);
extern u16 func_8002F4E4(SoundEntry *);
extern void func_8002F714(u16, s32);
extern SoundEntry D_8006451C[];
extern s16 D_800A4EFC;
extern u8 D_800A4F17;
extern u8 D_800A4F1B;
extern u8 D_8006A980;
extern u16 D_8006A982;
extern u8 D_8006A984;
extern u8 D_8006A988[];

s32 func_8002DC68(s32 id, u16 arg) {
    SoundEntry *entry;
    u16 redirect;
    u8 *pause;
    u8 was_paused;
    s32 ret;

    if (D_800A4EFC != 0) {
        return 0;
    }
    entry = &D_8006451C[(u16)id];
    if (D_800A4F1B == 0 && !(entry->kind & 0x80)) {
        return 0;
    }
    if ((entry->kind & 0x3F) == 6) {
        redirect = func_8002F4E4(entry);
        if (redirect == 0) {
            return 0;
        }
        entry = &D_8006451C[redirect];
    }
    pause = &D_800A4F17;
    was_paused = *pause;
    *pause = 1;
    ret = 0;
    switch (entry->kind & 0x3F) {
    case 1:
        ret = func_80032048(id, entry, arg);
        break;
    case 9:
        ret = func_800324A4(id, entry, arg, 0);
        break;
    case 7:
        ret = func_800324A4(id, entry, arg, 1);
        break;
    case 10:
        func_800322A8(id, entry, arg);
        /* fall through */
    case 8:
        ret = func_80032774(id, entry, arg);
        break;
    case 5:
        ret = func_80034314(id, entry, arg);
        break;
    case 2:
        func_8002F248(entry, arg);
        break;
    case 11:
        if (D_8006A980 & 4) {
            D_8006A982 = D_8006A982 + D_8006A988[arg & 7] - 2;
        } else {
            D_8006A980 |= 4;
            D_8006A982 = D_8006A988[arg & 7];
        }
        if ((arg & 7) == 6) {
            D_8006A980 |= 1;
        } else if ((arg & 7) == 2) {
            D_8006A980 |= 2;
        }
        break;
    case 12:
        D_8006A980 |= 8;
        if (D_8006A984 < (arg & 0x7F)) {
            D_8006A984 = arg & 0x7F;
        }
        break;
    case 13:
        ret = func_80032048((0x8F4 << 16) | (u16)id, entry, 0xFFFF);
        break;
    case 14:
        ret = func_80032048((0xA8A << 16) | (u16)id, entry, 0xFFFF);
        break;
    case 15:
        ret = func_80032048((0x896 << 16) | (u16)id, entry, 0xFFFF);
        break;
    case 17:
        func_8002F714(entry->bus, arg);
        break;
    case 16:
        ret = func_80032048((0xB7D << 16) | (u16)id, entry, 0xFFFF);
        break;
    case 18:
        ret = func_80032048((0x4F3 << 16) | (u16)id, entry, 0xFFFF);
        break;
    }
    D_800A4F17 = was_paused;
    return ret;
}
