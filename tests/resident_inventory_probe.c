#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "psx_types.h"

enum { FIRST = 0x2f, SLOTS = 12, LAST = FIRST + SLOTS, VARIABLE_BYTES = 256 };

u8 D_800BA1B8[VARIABLE_BYTES];
s16 D_80078EAC;

u32 func_800D0EC4(void);
void func_800D0F0C(u32 slot, u32 item);
s32 func_800D0F4C(s32 item);
s32 func_800D0F8C(s32 item);

static unsigned first_value(const u8 *variables, u8 value)
{
    unsigned slot;
    for (slot = FIRST; slot < LAST; ++slot)
        if (variables[slot] == value)
            return slot;
    return 0;
}

static void check_search_and_removal(void)
{
    u8 expected[VARIABLE_BYTES];
    unsigned mask, i;
    for (mask = 0; mask < (1u << SLOTS); ++mask) {
        memset(expected, 0xa7, sizeof expected);
        for (i = 0; i < SLOTS; ++i)
            expected[FIRST + i] = (mask & (1u << i)) ? (u8)(1u + i % 3u) : 0;
        memcpy(D_800BA1B8, expected, sizeof expected);
        assert(func_800D0EC4() == first_value(expected, 0));
        for (i = 0; i <= 4; ++i) {
            unsigned found = first_value(expected, (u8)i);
            s32 item = (s32)i - 256;
            assert(func_800D0F8C(item) == (s32)found);
            assert(memcmp(D_800BA1B8, expected, sizeof expected) == 0);
            assert(func_800D0F4C(item) == (s32)found);
            if (found)
                expected[found] = 0;
            assert(memcmp(D_800BA1B8, expected, sizeof expected) == 0);
        }
    }
}

static void check_acquisition_stamps(void)
{
    u8 expected[VARIABLE_BYTES];
    unsigned tick;
    memset(expected, 0xc3, sizeof expected);
    memset(expected + FIRST, 0, SLOTS);
    memcpy(D_800BA1B8, expected, sizeof expected);
    for (tick = 0; tick < 0x10000u; ++tick) {
        unsigned slot = FIRST + tick % SLOTS;
        unsigned stamp = 0x14u + 2u * (slot - FIRST);
        unsigned item = 0x123400u | ((tick * 13u) & 0xffu);
        D_80078EAC = (s16)((s32)tick - 32768);
        func_800D0F0C(slot, item);
        expected[slot] = (u8)item;
        expected[stamp] = (u8)D_80078EAC;
        expected[stamp + 1] = (u8)((u16)D_80078EAC >> 8);
        assert(memcmp(D_800BA1B8, expected, sizeof expected) == 0);
    }
}

static void check_fill_and_reuse(void)
{
    unsigned i, slot, stamp;
    memset(D_800BA1B8, 0, sizeof D_800BA1B8);
    for (i = 0; i < SLOTS; ++i) {
        slot = func_800D0EC4();
        assert(slot == FIRST + i);
        D_80078EAC = (s16)(256u + i);
        func_800D0F0C(slot, 32u + i);
    }
    assert(func_800D0EC4() == 0);
    slot = (unsigned)func_800D0F4C(37);
    assert(slot == FIRST + 5);
    stamp = 0x14u + 2u * (slot - FIRST);
    assert(D_800BA1B8[stamp] == 5 && D_800BA1B8[stamp + 1] == 1);
    assert(func_800D0EC4() == slot);
    D_80078EAC = -2;
    func_800D0F0C(slot, 0x1ff);
    assert(func_800D0F8C(-1) == (s32)slot);
    assert(func_800D0F8C(37) == 0);
    assert(D_800BA1B8[stamp] == 0xfe && D_800BA1B8[stamp + 1] == 0xff);
}

int main(void)
{
    check_search_and_removal();
    check_acquisition_stamps();
    check_fill_and_reuse();
    puts("Resident inventory passed all occupancy masks and acquisition-day bit patterns.");
    return 0;
}
