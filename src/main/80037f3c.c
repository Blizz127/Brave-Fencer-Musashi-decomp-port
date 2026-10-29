/* Owner re-derivation from retail assembly (clean input set). */
/* Releases every keyed channel that has started playing: hands its voice block to func_8003B250 and clears its attributes and start/pending flags. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x10];
    s32 voice_bit;
    s32 attr;
    u8 pad18[0x5A - 0x18];
    u8 keyed;
    u8 busy;
    u8 active;
    u8 started;
    u8 pending;
    u8 pad5F;
} Channel;

extern void func_8003B250(s32, s32 *);
extern Channel D_800C6DD0[16];

void func_80037F3C(void) {
    Channel *ch;
    s32 i;

    ch = D_800C6DD0;
    for (i = 0; i < 16; i++, ch++) {
        if (ch->keyed != 0 && ch->started != 0) {
            func_8003B250(i, &ch->voice_bit);
            ch->attr = 0;
            ch->started = 0;
            ch->pending = 0;
        }
    }
}
