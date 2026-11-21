#include <assert.h>
#include <stddef.h>
#include "sound_voice_release.h"

u8 D_800A4F17;
s32 D_800C7D20;
s32 D_80073140[32];
VoiceSlot D_800A4C2C[8];
static Voice *current_voice;
static u32 call_data;
static s32 channel_bits;
static unsigned masks, slides, callbacks;
static unsigned expect_mask, expect_slide;

void func_8002EFF8(s32 mode, s32 bits)
{
    assert(D_800A4F17 == 1 && !masks && !slides && !callbacks);
    assert(mode == 0 && bits == channel_bits);
    ++masks;
}
void func_80031988(PitchSlide *slide)
{
    assert(D_800A4F17 == 1 && masks == 1 && !slides && !callbacks);
    assert(slide == &current_voice->prefix.slide);
    ++slides;
}
static void released(u8 slot, u32 argument)
{
    assert(D_800A4F17 == 1 && masks == expect_mask && slides == expect_slide);
    assert(slot == current_voice->slot && argument == call_data);
    assert(current_voice->callback == released && !callbacks);
    ++callbacks;
    current_voice->state = (u8)-1;
}
int main(void)
{
    u32 seed = 37;
    for (unsigned i = 0; i < 2048; ++i) {
        seed = seed * 1664525u + 1013904223u;
        Voice voice = {0};
        const unsigned pending = i & 1u;
        const unsigned eligible = (i >> 1) & 1u;
        const unsigned masked = (i >> 2) & 1u;
        const unsigned requested = (i >> 3) & 1u;
        const unsigned callback = (i >> 4) & 1u;
        const unsigned forced = !pending && eligible && masked;
        const unsigned release_slot = !pending && (forced || requested);
        const u8 incoming_state = eligible ? (u8)(2 + seed % 253) : (u8)(i & 1);
        const u8 old_busy = (u8)(seed >> 17);
        const unsigned number = (seed >> 8) % 23;
        const u32 old_flags = seed;
        const u8 old_slot_release = (u8)(seed >> 3);
        s32 mode = (s32)((seed & 0xffff0000u) | (requested ? (seed | 1u) & 0xffffu : 0));
        current_voice = &voice;
        voice.prefix.channel.pending = pending;
        voice.prefix.channel.number = (u16)number;
        voice.state = incoming_state;
        voice.slot = (u8)(seed % 8);
        voice.release = (u8)(seed >> 16);
        const u8 old_release = voice.release;
        voice.callback_data = call_data = seed;
        voice.callback = callback ? released : NULL;
        channel_bits = D_80073140[number] = 1 << number;
        D_800C7D20 = masked ? channel_bits : 0;
        D_800A4F17 = old_busy;
        VoiceSlot *slot = &D_800A4C2C[voice.slot];
        slot->flags = old_flags;
        slot->release = old_slot_release;
        masks = slides = callbacks = 0;
        expect_mask = !pending;
        expect_slide = !pending && !forced && !requested;
        func_80030D80(&voice, mode);
        assert(D_800A4F17 == old_busy && voice.callback == NULL);
        assert(masks == expect_mask && slides == expect_slide && callbacks == callback);
        assert(voice.prefix.channel.pending == 0);
        assert(voice.state == (pending || forced ? 0 : callback ? (u8)-1 : incoming_state));
        assert(voice.release == (pending || forced ? old_release : (u8)requested));
        assert(slot->release == (release_slot ? 1 : old_slot_release));
        assert(slot->flags == (release_slot ? old_flags & ~(3u << 17) : old_flags));
    }
    return 0;
}
