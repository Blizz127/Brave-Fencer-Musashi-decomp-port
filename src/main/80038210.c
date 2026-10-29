/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Starts a MIDI sequence on the first free of the two sequence players: parses the header and track, resets the player and returns its index (or -1). */
#include "psx_types.h"

typedef struct {
    u32 cursor; /* guest u8 * */
    u32 start; /* guest u8 * */
    u8 pad8[0x1BA - 8];
    u8 mute[16];
    u8 pad1CA[0x1D8 - 0x1CA];
    u32 buffer; /* guest u8 * */
    u32 programs; /* guest u8 * */
    u32 tones; /* guest u8 * */
    u8 pad1E4[4];
    s32 param;
    s16 id;
    u8 pad1EE[0x1FA - 0x1EE];
    u8 state;
    u8 used;
} SeqPlayer;

extern s32 func_80038698(SeqPlayer *);
extern s32 func_800387C0(SeqPlayer *);
extern void func_80038838(SeqPlayer *);
extern u8 *D_800A6428;
extern SeqPlayer D_800B9CD8[2];
extern u8 D_800C1320[];

s32 func_80038210(u8 *data, s16 id, s32 param) {
    SeqPlayer *seq;
    s32 i;
    s32 k;
    u8 *p;

    seq = D_800B9CD8;
    for (i = 0; i < 2; i++, seq++) {
        if (seq->used == 0) {
            D_800A6428 = seq->mute;
            seq->cursor = (u32)data;
            seq->id = id;
            seq->param = param;
            seq->buffer = (u32)D_800C1320;
            seq->programs = (u32)(D_800C1320 + 0x20);
            seq->tones = (u32)(D_800C1320 + 0x820);
            if (func_80038698(seq) != 0) {
                return -1;
            }
            if (func_800387C0(seq) != 0) {
                return -1;
            }
            func_80038838(seq);
            seq->start = seq->cursor;
            seq->used = 1;
            seq->state = 0;
            /* clear mute[15..0]; mute[] lives at offset 0x1BA */
            for (k = 15, p = (u8 *)seq + 15; k >= 0; k--, p--) {
                p[0x1BA] = 0;
            }
            return i;
        }
    }
    return -1;
}
