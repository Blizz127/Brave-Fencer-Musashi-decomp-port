/* Owner re-derivation from retail assembly (clean input set). */
/* Resets the 16 sound channels and the two sequence players, then starts the sound driver with func_8003D424. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x10];
    s32 voice_bit;
    s32 attr;
    u8 pad18[0x1C - 0x18];
    s16 vol_left;
    s16 vol_right;
    u8 pad20[0x5A - 0x20];
    u8 keyed;
    u8 busy;
    u8 active;
    u8 started;
    u8 pending;
    u8 pad5F;
} Channel;

typedef struct {
    s16 id;
    u8 pad2[0xA - 2];
    u8 playing;
    u8 paused;
    u8 padC[0x1FC - 0xC];
} SeqPlayer;

extern void func_8003D424(u8 *);
extern s32 D_80073140[];
extern u8 *D_800762B0;
extern u8 D_80079A68[];
extern u8 D_800A4EFE[];
extern SeqPlayer D_800B9EC8[2];
extern Channel D_800C6DD0[16];

void func_80037D98(void) {
    s32 *bits;
    s32 bit;
    s32 i;

    D_800762B0 = D_800A4EFE;
    for (i = 0, bits = D_80073140; i < 16; i++) {
        bit = *bits++;
        D_800C6DD0[i].keyed = 0;
        D_800C6DD0[i].busy = 0;
        D_800C6DD0[i].active = 0;
        D_800C6DD0[i].started = 0;
        D_800C6DD0[i].vol_left = 0;
        D_800C6DD0[i].vol_right = 0;
        D_800C6DD0[i].attr = 0;
        D_800C6DD0[i].pending = 0;
        D_800C6DD0[i].voice_bit = bit;
    }
    for (i = 0; i < 2; i++) {
        D_800B9EC8[i].paused = 0;
        D_800B9EC8[i].playing = 0;
        D_800B9EC8[i].id = i;
    }
    func_8003D424(D_80079A68);
}
