#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "../src/main/8002c8f4.c"

/* The test linker binds address-named game globals to this single allocation.
 * Two different C declarations can consequently observe the same field. */
_Alignas(16) volatile unsigned char sound_test_state[0x27d30];
static unsigned service;
static unsigned fill;
static void expect(unsigned n) { assert(service == n); ++service; }
static unsigned byte_at(unsigned n) { return sound_test_state[n]; }
static unsigned half_at(unsigned n) { return byte_at(n) | (byte_at(n+1) << 8); }
static uint32_t word_at(unsigned n) {
    return (uint32_t)half_at(n) | ((uint32_t)half_at(n+2) << 16);
}

void func_8003A424(void) { expect(0); }
void func_8003D518(void) { expect(1); }
void func_8003C598(SoundMix *mix)
{
    expect(2);
    assert((uintptr_t)mix == (uintptr_t)&sound_test_state[0x4ea4]);
    assert(word_at(0x4ea4) == 0x23cf && word_at(0x4ebc) == 1);
    assert(half_at(0x4ea8) == 0x3fff && half_at(0x4eaa) == 0x3fff);
    assert(half_at(0x4eb4) == 0x3fff && half_at(0x4eb6) == 0x3fff);
    assert(word_at(0x4eac) == 0 && word_at(0x4eb8) == 0 && word_at(0x4ec8) == 0);
    assert(byte_at(0x4eb0) == fill);
}
void func_8003BE24(int mode) { expect(3); assert(mode == 1); }
void func_8002D1F0(Half count) { expect(4); assert(count == 4); }
void func_8003B280(int mode) { expect(5); assert(mode == 1); }
void func_80037D98(void) { expect(6); }

void func_8002CC4C(void)
{
    expect(7);
    assert(word_at(0x4e68) == 60 && half_at(0x4e6c) == 47 && half_at(0x4e6e) == 47);
    assert(word_at(0x4654) == 0x10000 && word_at(0x466c) == 0x14000);
    assert(word_at(0x4684) == 0x18000 && word_at(0x469c) == 0x39f00);
    for (unsigned i = 0; i < 5; ++i) {
        unsigned slot = 0x4640+24*i;
        assert(half_at(slot+2) == 0xffff && half_at(slot+4) == 0 && byte_at(slot+16) == 1);
    }
    for (unsigned i = 0; i < 8; ++i) {
        unsigned channel = 0x4988+84*i, voice = 0x4c68+72*i;
        assert(word_at(0x64b0+12*i) == 0 && byte_at(0x64b4+12*i) == fill);
        assert(half_at(0x46e8+84*i) == 0 && byte_at(0x46ea+84*i) == fill);
        assert(word_at(channel+4) == 0 && word_at(channel+64) == 0);
        assert(half_at(channel+10) == i+16 && byte_at(channel+81) == i);
        assert(byte_at(channel+78) == 0 && byte_at(channel+80) == 0);
        assert(word_at(voice) == i+16 && byte_at(voice+4) == 0 && byte_at(voice+5) == 0);
        assert(byte_at(voice+6) == fill);
    }
    sound_test_state[0x4f1b] = 0xe7;
    sound_test_state[0x4f1c] = 0x39;
}

void func_8002FAE0(void)
{
    expect(8);
    assert(half_at(0x4efa) == 127 && half_at(0x4ef8) == 127 && byte_at(0x4f1b) == 1);
    assert(half_at(0x4ee0) == 0x4000 && half_at(0x4ee4) == 0x4000);
    assert(word_at(0x2b98) == 0 && word_at(0x27d20) == 0 && word_at(0x27d2c) == 0);
    assert(byte_at(0x4f1c) == 0x39);
}
void func_80037CC8(void) { expect(9); }
void func_8003BE74(int mode, unsigned int mask) { expect(10); assert(mode == 0 && mask == 0xffffffu); }
void func_8003B1EC(SoundOption *option)
{
    expect(11);
    assert(option->enabled == 1 && option->value == 0);
    sound_test_state[0x4f1e] = 0x57;
}
void func_80034C24(void)
{
    expect(12);
    assert(byte_at(0x4f1c) == 0 && byte_at(0x4f1e) == 0);
}
void func_80037004(void) { expect(13); }

int main(void)
{
    static const unsigned fills[] = {0, 0x55, 0xaa, 0xff};
    _Static_assert(sizeof(SoundSlot) == 24 && sizeof(SoundChannel) == 84, "channel layout");
    _Static_assert(sizeof(SoundBank) == 12 && sizeof(SoundEntry) == 84, "record strides");
    _Static_assert(sizeof(SoundVoice) == 72 && sizeof(SoundOption) == 8, "voice and option layout");
    for (unsigned n = 0; n < sizeof fills/sizeof fills[0]; ++n) {
        service = 0;
        fill = fills[n];
        for (size_t i = 0; i < sizeof sound_test_state; ++i) sound_test_state[i] = fill;
        func_8002C8F4();
        assert(service == 14 && byte_at(0x27d1f) == fill && byte_at(0x4f1f) == fill);
    }
    return 0;
}
