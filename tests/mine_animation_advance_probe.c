#include "actor_position_updates.h"
#include "mine_animation_frames.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

extern void func_8012A988(Obj8014A2E4 *actor);

Obj8014A2E4 D_801202A0[96];
s16 D_801270C0;
static Obj8014A2E4 *tracked;
static const u32 key = UINT32_C(0x96a51022);
static const u32 alternate = UINT32_C(0xc4621034);
static s16 frames[24], replacement[24];
static unsigned count, reads, applies, change_at, fail_at;
static u32 changed_key, applied_index;
static u16 applied_flags, applied_delay;

static u16 half(const Obj8014A2E4 *a, unsigned offset)
{
    const u8 *p = (const u8 *)a + offset;
    return (u16)(p[0] | (u16)p[1] << 8);
}
static u32 word(const Obj8014A2E4 *a, unsigned offset)
{
    return (u32)half(a, offset) | (u32)half(a, offset + 2) << 16;
}
static void put_half(Obj8014A2E4 *a, unsigned offset, u16 value)
{
    u8 *p = (u8 *)a + offset;
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}
static void put_word(Obj8014A2E4 *a, unsigned offset, u32 value)
{
    put_half(a, offset, (u16)value);
    put_half(a, offset + 2, (u16)(value >> 16));
}
static void start(Obj8014A2E4 *a, u16 flags, u16 delay, u32 index)
{
    memset(a, 0, sizeof(*a));
    tracked = a;
    put_half(a, 0, 1);
    put_half(a, 0x72, flags);
    put_word(a, 0x90, key);
    put_word(a, 0x94, index);
    put_half(a, 0x98, delay);
    count = 24;
    reads = applies = change_at = fail_at = 0;
    changed_key = alternate;
}
int bfm_animation_frame_operation(u32 address, s16 *operation)
{
    u32 base = address >= alternate ? alternate : key;
    u32 delta = address - base;
    unsigned i = (delta - 4u) / 8u;
    reads++;
    if (reads == fail_at || delta < 4 || (delta - 4u) % 8 || i >= count)
        return 0;
    *operation = (base == key ? frames : replacement)[i];
    if (reads == change_at)
        put_word(tracked, 0x90, changed_key);
    return 1;
}
void func_8012AAAC(Obj8014A2E4 *a)
{
    applies++;
    applied_index = word(a, 0x94);
    applied_flags = half(a, 0x72);
    applied_delay = half(a, 0x98);
    ((u8 *)a)[0x89]++;
}

static void verify_periods(void)
{
    Obj8014A2E4 *a = D_801202A0;
    unsigned n;
    for (n = 0; n <= UINT16_MAX; n++) {
        unsigned next = n ? (n - 1u) & UINT16_MAX : 0;
        int tick = n != 0 && (next == 0 || next >= 0x8000);
        start(a, 0x6000, (u16)n, UINT32_MAX);
        func_8012A988(a);
        assert(half(a, 0x72) == 0 && half(a, 0x98) == next);
        assert(reads == 0 && applies == (unsigned)tick);
        assert(word(a, 0x94) == (tick ? 0 : UINT32_MAX));
        if (tick)
            assert(applied_index == 0 && applied_flags == 0 && applied_delay == next);
    }
    start(a, 0x6600, 1, 0);
    ((u8 *)a)[0xc1] = 0x11;
    func_8012A988(a);
    assert(!reads && !applies && half(a, 0x72) == 0x600 && half(a, 0x98) == 1);
    put_half(a, 0, 0);
    ((u8 *)a)[0xc1] = 2;
    func_8012A988(a);
    assert(!reads && !applies && half(a, 0x98) == 1);
    put_half(a, 0, 9);
    put_word(a, 0x90, 0);
    func_8012A988(a);
    assert(!reads && !applies && half(a, 0x98) == 0);
    func_8012A988(0);
}

static void verify_directions(void)
{
    Obj8014A2E4 *a = D_801202A0;
    unsigned bits;
    u32 i;
    for (bits = 0; bits < 32; bits++) {
        for (i = 0; i < 2; i++) {
            u32 initial = (UINT32_C(1) << bits) - i;
            u32 previous = initial - 1;
            start(a, 0x6600, 1, initial);
            func_8012A988(a);
            assert(applies == 1 && !reads && applied_delay == 0);
            assert(applied_index == (previous >> 31 ? 1 : previous));
            assert(applied_flags == (previous >> 31 ? 0x200 : 0x600));
        }
    }
    for (i = 0; i < 24; i++) {
        int end;
        for (end = -2; end <= 0; end++) {
            unsigned j;
            for (j = 0; j < i; j++)
                frames[j] = (j & 1) ? -50 : (s16)(j + 2);
            frames[i] = (s16)end;
            start(a, 0x6200, 1, 0);
            func_8012A988(a);
            assert(reads == i + 2 && applies == (unsigned)(end == -1));
            assert(word(a, 0x94) == (end == -1 ? i - 1u : 0u));
            assert(half(a, 0x98) == 0);
        }
    }
    start(a, 0x200, 1, 0x80000000u);
    func_8012A988(a);
    assert(!reads && applies == 1 && applied_index == 0x7fffffffu);
}

static void verify_changed_list(void)
{
    Obj8014A2E4 *a = D_801202A0;
    unsigned i;
    frames[0] = 7;
    frames[1] = -4;
    frames[2] = -1;
    replacement[2] = -2;
    start(a, 0x200, 0xffffu, 0);
    change_at = 1;
    func_8012A988(a);
    assert(reads == 4 && !applies && !word(a, 0x94) && !half(a, 0x98));
    replacement[2] = -1;
    frames[2] = 0;
    start(a, 0x200, 1, 0);
    change_at = 1;
    func_8012A988(a);
    assert(reads == 4 && applies == 1 && applied_index == 1);
    for (i = 1; i <= 4; i++) {
        start(a, 0x200, 1, 0);
        fail_at = i;
        func_8012A988(a);
        assert(reads == i && !applies && !word(a, 0x94) && !half(a, 0x98));
    }
    for (i = 0; i < 3; i++) {
        start(a, 0x200, 1, 0);
        changed_key = i == 0 ? 0 : i == 1 ? alternate + 1 : 0xfffffff2u;
        change_at = 1;
        func_8012A988(a);
        assert(reads == 3 && !applies && !word(a, 0x94) && !half(a, 0x98));
    }
}

static void verify_prepass(void)
{
    unsigned mode, i;
    for (mode = 0; mode < 2; mode++) {
        for (i = 0; i < 96; i++)
            start(D_801202A0 + i, (i & 1) ? 0x1000 : 0, 1, i);
        D_801270C0 = (s16)mode;
        func_8012A908();
        assert(applies == (mode ? 96u : 48u) && !reads);
        for (i = 0; i < 96; i++) {
            unsigned visited = mode || (i & 1);
            assert(word(D_801202A0 + i, 0x94) == i + visited);
            assert(((u8 *)(D_801202A0 + i))[0x89] == visited);
        }
    }
}

int main(void)
{
    verify_periods();
    verify_directions();
    verify_changed_list();
    verify_prepass();
    return 0;
}
