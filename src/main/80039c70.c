/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Executes a sequencer meta event on a channel (music change, sound request, loop start, fade sync), then clears the channel's pending-event bits. */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u8 state;
    u8 flags;
    u8 event;
    u8 pad9[0x1A - 9];
} SeqChannel;

typedef struct {
    u32 cursor; /* guest u8 * */
    u8 pad4[0xC - 4];
    u32 loop_start; /* guest u8 * */
    u8 pad10[0x1A - 0x10];
    SeqChannel channels[16];
    u8 pad1BA[0x1F5 - 0x1BA];
    u8 loop_count;
    u8 looping;
    u8 status;
    u8 pad1F8;
    u8 loop_end;
} Sequence;

typedef struct {
    s32 kind;
    s32 value;
    u16 id;
    u16 id2;
} SoundRequest;

extern s32 D_8006B008[];
extern u16 D_8006ADD8[];
extern s32 D_80079A68;
extern SoundRequest D_800A4ECC;
extern u16 D_800A4F20[];
extern u16 D_800A4F22;
extern void func_8003B45C(SoundRequest *);

void func_80039C70(Sequence *seq, s16 channel, s16 arg) {
    SeqChannel *ch;
    s32 value;
    u16 fade;

    ch = &seq->channels[channel];
    if ((*(u16 *)&ch->state & 0x300) != 0x300) {
        seq->loop_end = 1;
        return;
    }
    switch (ch->event) {
    case 16:
        if (D_80079A68 != D_8006B008[arg]) {
            value = D_8006B008[arg];
            D_80079A68 = value;
            D_800A4ECC.kind = 1;
            D_800A4ECC.value = value;
            func_8003B45C(&D_800A4ECC);
        }
        break;
    case 17:
        D_800A4ECC.kind = 6;
        fade = D_8006ADD8[arg];
        D_800A4ECC.id = fade;
        D_800A4ECC.id2 = fade;
        func_8003B45C(&D_800A4ECC);
        break;
    case 24:
        seq->loop_count = arg;
        seq->looping = 1;
        seq->loop_start = seq->cursor;
        break;
    case 26:
        if (D_800A4F20[0] != 0) {
            seq->loop_end = 1;
            fade = D_800A4F20[0];
            D_800A4F20[0] = 0;
            D_800A4F22 = fade;
        }
        break;
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 25:
        break;
    }
    ch->flags &= 0xFC;
}
