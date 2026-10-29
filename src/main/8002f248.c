/* Owner re-derivation from retail assembly (clean input set). */
/* Plays the sound effect for an event type: random picks from small sets, a caller-supplied id, or a +-0x100 pitch bend on the voices of the found sound. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xE];
    u8 voice_used[8];
    u8 pad16[0x20 - 0x16];
    s32 pitch;
    u8 pad24[0x28 - 0x24];
    s32 pitch_target;
    s32 pitch_step;
    s16 pitch_timer;
    u8 pad32[0x37 - 0x32];
    u8 flags;
    u8 pitch_done;
    u8 pad39[0x54 - 0x39];
} SoundChannel;

typedef struct {
    u8 pad0[3];
    u8 type;
} SoundEvent;

extern void func_8002DC68(s32, s32);
extern s32 func_800331D4(s32);
extern u16 D_8006A9F8[];
extern SoundChannel D_800A46E8[16];
extern u32 D_800A4EEC;

void func_8002F248(SoundEvent *event, s32 id) {
    SoundChannel *ch;
    s32 found;
    s32 i;
    s32 sound;
    SoundChannel *rec;

    switch (event->type) {
    case 0:
        D_800A4EEC = D_800A4EEC * 0x41C64E6D + 12345;
        switch ((D_800A4EEC >> 16) & 7) {
        case 0:
        case 3:
            sound = 1660;
            break;
        case 1:
        case 2:
            sound = 1673;
            break;
        case 4:
        case 5:
            sound = 1659;
            break;
        default:
            return;
        }
        break;
    case 1:
        sound = (u16)id;
        break;
    case 2:
        found = func_800331D4(2396);
        if (found != 0) {
            rec = &D_800A46E8[found - 1];
            ch = &D_800A46E8[8];
            for (i = 0; i < 8; i++) {
                if (rec->voice_used[i] != 0) {
                    ch[i].pitch_target = ch[i].pitch - 0x100;
                    ch[i].pitch_timer = 0;
                    ch[i].pitch_done = 0;
                    ch[i].pitch_step = -24;
                    ch[i].flags |= 1;
                }
            }
        }
        return;
    case 3:
        found = func_800331D4(2396);
        if (found != 0) {
            rec = &D_800A46E8[found - 1];
            ch = &D_800A46E8[8];
            for (i = 0; i < 8; i++) {
                if (rec->voice_used[i] != 0) {
                    ch[i].pitch_target = ch[i].pitch + 0x100;
                    ch[i].pitch_timer = 0;
                    ch[i].pitch_done = 0;
                    ch[i].pitch_step = 24;
                    ch[i].flags |= 1;
                }
            }
        }
        return;
    case 4:
        D_800A4EEC = D_800A4EEC * 0x41C64E6D + 12345;
        switch ((D_800A4EEC >> 16) & 7) {
        case 0:
        case 3:
            sound = 2813;
            break;
        case 1:
        case 2:
        case 5:
            sound = 2815;
            break;
        case 4:
            sound = 2814;
            break;
        default:
            return;
        }
        break;
    case 5:
        func_8002DC68(2822, 0);
        sound = 2823;
        break;
    case 6:
        func_8002DC68(2822, 0);
        sound = 3168;
        break;
    case 7:
        D_800A4EEC = D_800A4EEC * 0x41C64E6D + 12345;
        sound = *(u16 *)((u8 *)D_8006A9F8 + ((D_800A4EEC >> 15) & 0x1E));
        break;
    default:
        return;
    }
    func_8002DC68(sound, 0);
}
