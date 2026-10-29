/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Applies a sequencer controller event to a channel: loop markers (30) rewind the stream with a repeat count, other controllers record the value and flag the channel. */
#include "psx_types.h"

typedef struct {
    u8 pad0[5];
    u8 controller;
    u8 pad6;
    u8 flags;
    u8 param;
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

void func_80039DEC(Sequence *seq, s16 channel, s16 controller) {
    switch (controller) {
    case 20:
        seq->channels[channel].controller = controller;
        seq->channels[channel].param = 24;
        seq->channels[channel].flags |= 3;
        break;
    case 30:
        if (seq->looping == 0) {
            seq->loop_end = 1;
            break;
        }
        if (seq->loop_count != 0x7F) {
            if (seq->loop_count-- == 0) {
                break;
            }
        }
        seq->status = channel | 0xB0;
        seq->cursor = seq->loop_start;
        break;
    case 40:
        seq->channels[channel].controller = controller;
        seq->channels[channel].param = 26;
        seq->channels[channel].flags |= 3;
        break;
    default:
        seq->channels[channel].controller = controller;
        seq->channels[channel].flags |= 1;
        break;
    }
}
