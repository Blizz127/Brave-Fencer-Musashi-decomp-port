/* SPDX-License-Identifier: MIT */
typedef struct {
    unsigned char reserved[9];
    unsigned char used[16];
    unsigned char trailing;
} ChannelRow;

typedef struct {
    unsigned char prefix[26];
    ChannelRow rows[16];
    unsigned char padding[64];
    unsigned char state;
    unsigned char trailing;
} Sequence;

typedef struct {
    unsigned char current, pending;
    unsigned char padding[94];
} Channel;

extern Sequence D_800B9CD8[];
extern Channel D_800C6E2D[];
extern unsigned int D_80073140[];
extern unsigned char D_800A4F1D;
extern void func_8003916C(short channel);
extern void func_8002EFF8(int group, unsigned int mask);

void func_800383A4(int index)
{
    Sequence *sequence = &D_800B9CD8[index];
    ChannelRow *row = sequence->rows;
    int row_index, channel_index;

    D_800A4F1D = 1;
    sequence->state = 0;
    for (row_index = 0; row_index < 16; ++row_index, ++row) {
        unsigned char *used = row->used;
        Channel *channel;
        for (channel_index = 0, channel = D_800C6E2D;
             channel_index < 16; ++channel_index, ++channel) {
            if (*used++) {
                channel->pending = 1;
                channel->current = 0;
                func_8003916C(channel_index);
                func_8002EFF8(0, D_80073140[channel_index]);
            }
        }
    }
    D_800A4F1D = 0;
}
