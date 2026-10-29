/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame sound voice flush: deferred key-offs, pending SPU key/flag masks and voice releases. */
#include "psx_types.h"

typedef struct {
    s32 unk0;
    u32 flags;
    u8 pad8[0x38];
    s32 voice;
    u8 release;
    u8 keyOff;
    u8 pad46[2];
} SndVoice;

extern SndVoice D_800A4C28[];
extern u32 D_800A2B98;
extern u32 D_800A2BA0;
extern u32 D_800C7D20;
extern u32 D_800C7D2C;

void func_8003D3B4(s32 voice, s32 mode);
void func_8003C23C(s32 on, u32 bits);
void func_8003BE74(s32 on, u32 bits);
void func_8003B250(s32 voice, SndVoice *v);

void func_8002D320(void)
{
    s32 i;
    u32 bits;
    SndVoice *v;

    v = D_800A4C28;
    for (i = 0; i < 8; i++, v++) {
        if (v->keyOff) {
            if (v->release == 0 || !(v->flags & 0x60000)) {
                func_8003D3B4(v->voice, 1);
            }
            v->keyOff = 0;
        }
    }
    bits = D_800A2B98 & 0xFF0000;
    if (bits) {
        func_8003C23C(0, bits);
        D_800A2B98 &= 0xFFFF;
    }
    bits = D_800A2BA0 & 0xFF0000;
    if (bits) {
        func_8003BE74(0, bits);
        D_800A2BA0 &= 0xFFFF;
    }
    v = D_800A4C28;
    for (i = 0; i < 8; v++, i++) {
        if (v->release) {
            v->release = 0;
            func_8003B250(v->voice, v);
            v->flags = 0;
        }
    }
    bits = D_800C7D20 & 0xFF0000;
    if (bits) {
        func_8003C23C(1, bits);
        D_800C7D20 &= 0xFFFF;
    }
    bits = D_800C7D2C & 0xFF0000;
    if (bits) {
        func_8003BE74(1, bits);
        D_800C7D2C &= 0xFFFF;
    }
}
