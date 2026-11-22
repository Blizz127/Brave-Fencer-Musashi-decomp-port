#include "forest_projectile_contact.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

ForestProjectileOffset D_801D8BB0;
static u8 projectile, calls;
static u16 expected_attack, expected_mask, expected_power;
static s32 expected_flags, expected_result;
static ForestProjectileOffset *expected_offset;
static unsigned power_lookups;

s32 func_80156370(u16 attack)
{
    assert(attack == expected_attack);
    ++power_lookups;
    return expected_power;
}

s32 func_80163534(void *effect, u16 attack, u16 power, s32 flags,
                  u16 mask, ForestProjectileOffset *offset)
{
    assert(effect == &projectile);
    assert(attack == expected_attack && power == expected_power);
    assert(flags == expected_flags && mask == expected_mask);
    assert(offset != &D_801D8BB0);
    if (expected_offset) assert(offset == expected_offset);
    assert(!memcmp(offset->bytes, D_801D8BB0.bytes, sizeof(offset->bytes)));
    offset->bytes[5] ^= 255;
    ++calls;
    return expected_result;
}

int main(void)
{
    static const struct {
        u16 attack, mask, power;
        s32 flags, result;
    } cases[] = {
        {16, 512, 4, 128, 1},
        {0, 0, 0, 0, 0},
        {65535, 32768, 65535, INT_MIN, -234},
        {32768, 65535, 0x81, INT_MAX, 4}
    };
    for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        ForestProjectileOffset input, before;
        for (unsigned j = 0; j < sizeof(input.bytes); ++j)
            input.bytes[j] = (u8)(i * 59 + j * 13 + 3);
        D_801D8BB0 = input;
        before = input;
        expected_attack = cases[i].attack;
        expected_mask = cases[i].mask;
        expected_power = cases[i].power;
        expected_flags = cases[i].flags;
        expected_result = cases[i].result;
        expected_offset = NULL;
        assert(func_80163408(&projectile, expected_attack, expected_flags,
                             expected_mask) == expected_result);
        assert(!memcmp(&D_801D8BB0, &before, sizeof(before)));
        expected_offset = &input;
        assert(func_8016345C(&projectile, expected_attack, expected_flags,
                             expected_mask, &input) == expected_result);
        assert(input.bytes[5] == (u8)(before.bytes[5] ^ 255u));
        assert(!memcmp(&D_801D8BB0, &before, sizeof(before)));
    }
    assert(calls == 8 && power_lookups == 8);
    return 0;
}
