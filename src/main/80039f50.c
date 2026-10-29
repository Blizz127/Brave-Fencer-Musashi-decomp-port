/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Sequencer control-change: reads controller and value from the stream and applies volume, pan, meta-event or loop/marker controllers to the channel. */
#include "psx_types.h"

typedef struct {
    u8 index;
    u8 volume;
    s16 expression;
    u8 pan;
    u8 pad5;
    u8 meta;
    u8 flags;
    u8 meta_next;
    u8 key_on[16];
    u8 pad19;
} SeqChannel;

typedef struct {
    u32 cursor; /* guest u8 * */
    u8 pad4[0x1A - 4];
    SeqChannel channels[16];
} Sequence;

extern void func_80039C70(Sequence *, s16, s16);
extern void func_80039DEC(Sequence *, s16, s16);

void func_80039F50(Sequence *seq, s16 channel) {
    s16 controller;
    u8 value;

    controller = *(u8 *)seq->cursor++;
    value = *(u8 *)seq->cursor++;
    switch (controller) {
    case 6:
        func_80039C70(seq, channel, value);
        break;
    case 7:
        seq->channels[channel].volume = value;
        break;
    case 8:
        break;
    case 10:
        seq->channels[channel].pan = value;
        break;
    case 98:
        seq->channels[channel].meta = value;
        seq->channels[channel].meta_next = value + 1;
        seq->channels[channel].flags |= 2;
        break;
    case 99:
        func_80039DEC(seq, channel, value);
        break;
    }
}
