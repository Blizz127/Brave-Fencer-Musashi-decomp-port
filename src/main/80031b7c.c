/* Owner re-derivation from retail assembly (clean input set). */
/* Starts the release phase of every playing channel that has a release pending and has not started it yet. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x4A];
    s16 timer;
    u8 releasing;
    u8 release_pending;
    u8 playing;
    u8 pad4F[0x54 - 0x4F];
} SoundChannel;

extern SoundChannel D_800A4988[8];

void func_80031B7C(void) {
    SoundChannel *ch;
    s32 i;

    ch = D_800A4988;
    for (i = 0; i < 8; i++, ch++) {
        if (ch->playing != 0 && ch->releasing == 0 && ch->release_pending != 0) {
            ch->releasing = 1;
            ch->timer = 0x7FFF;
        }
    }
}
