/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Resets a sequence's 16 channel states and track state and restores its default tempo and flags. */
#include "psx_types.h"

typedef struct {
    u8 index;
    u8 volume;
    s16 expression;
    u8 pan;
    u8 pad5[2];
    u8 flags;
    u8 pad8;
    u8 key_on[16];
    u8 pad19;
} SeqChannel;

typedef struct {
    s16 scale;
    u8 pad2[4];
    u8 active;
    u8 pad7;
} SeqTrack;

typedef struct {
    u32 cursor; /* guest u8 * */
    u8 pad4[4];
    s32 running;
    u8 padC[0x12 - 0xC];
    SeqTrack tracks[1];
    SeqChannel channels[16];
    u8 pad1BA[0x1CC - 0x1BA];
    s32 tempo_c;
    s32 tempo_a;
    s32 tempo_b;
    u8 pad1D8[0x1E4 - 0x1D8];
    s16 bpm;
    u8 pad1E6[0x1F4 - 0x1E6];
    u8 flag1F4;
    u8 loop_count;
    u8 looping;
    u8 status;
    u8 flag1F8;
    u8 loop_end;
    u8 flag1FA;
} Sequence;

void func_80038838(Sequence *seq) {
    SeqChannel *ch;
    s32 i;
    s32 j;
    u8 *p;

    ch = seq->channels;
    for (i = 0; i < 16; i++, ch++) {
        ch->index = i;
        ch->expression = 0x40;
        ch->pan = 0x40;
        ch->flags = 0;
        ch->volume = 0x7F;
        p = ch->key_on;
        for (j = 0; j < 16; j++) {
            *p++ = 0;
        }
    }
    for (i = 0; i < 1; i++) {
        seq->tracks[i].active = 0;
        seq->tracks[i].scale = 0x4000;
    }
    seq->running = 1;
    seq->tempo_a = 3600;
    seq->tempo_b = 3600;
    seq->tempo_c = 3600;
    seq->flag1F4 = 0;
    seq->bpm = 120;
    seq->status = 0;
    seq->flag1F8 = 0;
    seq->loop_end = 0;
    seq->flag1FA = 0;
    seq->looping = 0;
}
