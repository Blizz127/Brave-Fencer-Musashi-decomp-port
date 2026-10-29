/* Owner re-derivation from retail assembly (clean input set). */
/* Flushes pending key-offs: for each keyed, not-started channel with a pending event, sends command 8 to its voice and clears the pending state. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x14];
    s32 attr;
    u8 pad18[0x5A - 0x18];
    u8 keyed;
    u8 busy;
    u8 active;
    u8 started;
    u8 pending;
    u8 pad5F;
} Channel;

extern void func_8003D3B4(s32, s32);
extern Channel D_800C6DD0[16];

void func_80037EA0(void) {
    Channel *ch;
    s32 i;

    ch = D_800C6DD0;
    for (i = 0; i < 16; i++, ch++) {
        if (ch->keyed != 0 && ch->started == 0 && ch->pending != 0) {
            func_8003D3B4(i, 8);
            ch->attr &= 0xFFF9FFFF;
            ch->pending = 0;
        }
    }
}
