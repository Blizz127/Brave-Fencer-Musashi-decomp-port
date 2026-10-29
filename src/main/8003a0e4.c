/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Sequencer pitch-bend event: stores the channel's bend value and recomputes the SPU pitch of every voice the channel is playing. */
#include "psx_types.h"

typedef struct {
    s32 bit;
    s32 attr;
} VoiceBlock;

typedef struct {
    u8 pad0[4];
    s16 note;
    u8 pad6[0x10 - 6];
    VoiceBlock voice;
    u8 pad18[0x24 - 0x18];
    s16 pitch;
    u8 pad26[0x54 - 0x26];
    s32 tune;
    u8 bend_down;
    u8 bend_up;
    u8 pad5A[0x5D - 0x5A];
    u8 started;
    u8 pad5E[0x60 - 0x5E];
} Channel;

typedef struct {
    u8 index;
    u8 volume;
    s16 bend;
    u8 pad4[5];
    u8 key_on[16];
    u8 pad19;
} SeqChannel;

typedef struct {
    u32 cursor; /* guest u8 * */
    u8 pad4[0x1A - 4];
    SeqChannel channels[16];
} Sequence;

extern u16 D_8006AB30[];
extern u16 D_8006ABD8[];
extern Channel D_800C6DD0[16];

void func_8003A0E4(Sequence *seq, s16 channel) {
    SeqChannel *chan;
    Channel *ch;
    u8 *key;
    s32 v;
    u32 pitch;
    s16 bend;
    s32 tune;
    u32 level;
    VoiceBlock *voice;

    chan = &seq->channels[channel];
    key = chan->key_on;
    chan->bend = *(u8 *)seq->cursor++ & 0x7F;
    for (v = 0; v < 16; v++) {
        if (*key++ != 0) {
            ch = &D_800C6DD0[v];
            bend = chan->bend;
            pitch = ch->note << 8;
            if (bend > 64) {
                pitch += (bend - 64) * ch->bend_up * 4;
            } else if (bend < 64) {
                pitch -= (64 - bend) * ch->bend_down * 4;
            }
            tune = ch->tune - 0x3C00;
            pitch -= tune;
            voice = &ch->voice;
            if (pitch > 0x5300) {
                ch->pitch = 0x3FFF;
            } else {
                level = D_8006AB30[pitch >> 8];
                level *= D_8006ABD8[(pitch & 0xFF) >> 1];
                level >>= 15;
                ch->pitch = level;
            }
            voice->attr |= 0x10;
            ch->started = 1;
        }
    }
}
