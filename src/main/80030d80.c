/* Reconstructed from the original game. Completes a channel release and runs its callback. */
#include "sound_voice_release.h"

void func_80030D80(Voice *voice, s32 release)
{
    u8 old = BUSY_BYTE;
    u8 clear_state;

    BUSY_BYTE = 1;
    if (voice->prefix.channel.pending) {
        voice->prefix.channel.pending = 0;
        clear_state = 1;
    } else if (voice->state >= 2 &&
               (D_800C7D20 & D_80073140[voice->prefix.channel.number])) {
        D_800A4C2C[voice->slot].release = 1;
        D_800A4C2C[voice->slot].flags &= ~0x60000u;
        clear_state = 1;
        func_8002EFF8(0, D_80073140[voice->prefix.channel.number]);
    } else {
        clear_state = 0;
        if ((s16)release) {
            voice->release = 1;
            D_800A4C2C[voice->slot].release = 1;
            D_800A4C2C[voice->slot].flags &= ~0x60000u;
            func_8002EFF8(0, D_80073140[voice->prefix.channel.number]);
        } else {
            s32 bits = D_80073140[voice->prefix.channel.number];
            voice->release = 0;
            func_8002EFF8(0, bits);
            func_80031988(&voice->prefix.slide);
        }
    }
    if (voice->callback) {
        voice->callback(voice->slot, voice->callback_data);
    }
    voice->callback = 0;
    if (clear_state) {
        voice->state = 0;
    }
    BUSY_BYTE = old;
}
