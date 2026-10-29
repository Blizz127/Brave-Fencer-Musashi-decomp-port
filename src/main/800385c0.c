/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Stops sequence player `player`: frees every channel it owns and clears its playing/paused flags. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1F0];
    s16 player;
} SeqOwner;

typedef struct {
    u8 pad0[0x50];
    u32 owner; /* guest SeqOwner * */
    u8 pad54[0x5A - 0x54];
    u8 keyed;
    u8 busy;
    u8 active;
    u8 pad5D[0x60 - 0x5D];
} Channel;

typedef struct {
    s16 id;
    u8 pad2[0xA - 2];
    u8 playing;
    u8 paused;
    u8 padC[0x1FC - 0xC];
} SeqPlayer;

extern SeqPlayer D_800B9EC8[2];
extern Channel D_800C6DD0[16];

void func_800385C0(s32 player) {
    Channel *ch;
    s32 i;

    ch = D_800C6DD0;
    for (i = 0; i < 16; i++, ch++) {
        if (ch->active != 0 && ((SeqOwner *)ch->owner)->player == player) {
            ch->active = 0;
            ch->keyed = 0;
        }
    }
    D_800B9EC8[player].playing = 0;
    D_800B9EC8[player].paused = 0;
}
