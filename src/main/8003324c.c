/* Owner re-derivation from retail assembly (clean room, clean input set).
 * Stops sequence record `index` of D_800A46E8 by its low 6 state bits: kind 1 clears each
 * pending voice flag (+0xE[j]) and, for a voice channel still marked live (+0x4E), zeroes its
 * +0x40 word and releases it via func_80030D80(channel, 0), then clears the state; kind 5
 * hands the record to func_80034650(record, 0). u16 index (entry andi): callers' prototypes
 * differ (s32 in 8002d6d8.c, u16 in 8003310c.c). */
#include "psx_types.h"

typedef struct {
    u16 state;
    u8 pad2[0xE - 2];
    u8 pending[8];
    u8 pad16[0x40 - 0x16];
    s32 w40;
    u8 pad44[0x4E - 0x44];
    u8 live;
    u8 pad4F[0x54 - 0x4F];
} SoundRec;

extern SoundRec D_800A46E8[16];
extern void func_80030D80(SoundRec *, s16);
extern void func_80034650(SoundRec *, u8);

void func_8003324C(u16 index) {
    SoundRec *p = &D_800A46E8[index];
    SoundRec *ch;
    s32 j;

    switch (p->state & 0x3F) {
    case 1:
        ch = &D_800A46E8[8];
        for (j = 0; j < 8; j++) {
            if (p->pending[j] != 0) {
                p->pending[j] = 0;
                ch[j].w40 = 0;
                if (ch[j].live != 0) {
                    func_80030D80(&ch[j], 0);
                }
            }
        }
        p->state = 0;
        break;
    case 5:
        func_80034650(p, 0);
        break;
    }
}
