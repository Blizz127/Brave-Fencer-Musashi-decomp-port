/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Stops sequence player `index`: releases every keyed voice of all 16 channels and rewinds the player to its start. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x5D];
    u8 started;
    u8 pending;
    u8 pad5F;
} Channel;

typedef struct {
    u8 pad0[9];
    u8 key_on[16];
    u8 pad19;
} SeqChannel;

typedef struct {
    u32 cursor; /* guest u8 * */
    u32 start; /* guest u8 * */
    u8 pad8[0x1A - 8];
    SeqChannel channels[16];
    u8 pad1BA[0x1FA - 0x1BA];
    u8 state;
    u8 used;
} SeqPlayer;

extern void func_8002EFF8(s32, s32);
extern void func_8003916C(s16);
extern s32 D_80073140[];
extern s8 D_800A4F1D[];
extern SeqPlayer D_800B9CD8[];
extern Channel D_800C6DD0[16];

void func_800384A8(s32 index) {
    SeqPlayer *seq;
    SeqChannel *chan;
    Channel *ch;
    u8 *key;
    s32 c;
    s32 v;

    seq = &D_800B9CD8[index];
    chan = seq->channels;
    D_800A4F1D[0] = 1;
    seq->state = 0;
    for (c = 0; c < 16; c++, chan++) {
        key = chan->key_on;
        ch = D_800C6DD0;
        for (v = 0; v < 16; v++, ch++) {
            if (*key++ != 0) {
                ch->pending = 1;
                ch->started = 0;
                func_8003916C(v);
                func_8002EFF8(0, D_80073140[v]);
            }
        }
    }
    D_800A4F1D[0] = 0;
    seq->cursor = seq->start;
}
