#include "psx_types.h"
#include <assert.h>
#include <stdint.h>

extern u16 func_8015616C(void *, u16);
s16 D_8012693A, D_801152B8;

static u16 row[6];
static int actor;
static u32 enabled, calls, flag_query[6], flag_count;
static s32 first, second, magic, random_value;
static u16 modifier_input;

u16 *func_801563EC(u32 index)
{
    assert(index == 37);
    return row;
}

s32 func_80029B4C(s16 a, s16 b)
{
    assert(a == (row[0] & 0xfff) && b == (s16)row[1]);
    calls = 10 * calls + 1;
    return first;
}

s32 func_80029BC8(s16 a, s16 b)
{
    assert(a == (s16)row[2] && b == (s16)row[3]);
    calls = 10 * calls + 2;
    return second;
}

s32 func_80029C44(s16 a, s16 b)
{
    assert(a == (row[0] & 0xfff) && b == (s16)row[1]);
    calls = 10 * calls + 3;
    return magic;
}

s32 func_8005C49C(void)
{
    calls = 10 * calls + 4;
    return random_value;
}

s32 func_80029178(u32 flag)
{
    unsigned pos = flag == 0x81 ? 5 : flag - 0x16;
    assert(flag_count < 6 && pos <= 5);
    flag_query[flag_count++] = flag;
    /* Only the low byte participates in the caller's branch. */
    return 0x100 | ((enabled >> pos) & 1u);
}

s32 func_8015640C(void *target, u16 strength)
{
    assert(target == &actor);
    calls = 10 * calls + 5;
    modifier_input = strength;
    return strength + 7;
}

static u16 apply(u16 flags, u16 type, u32 mask)
{
    row[0] = flags | 17;
    row[1] = 5;
    row[2] = 0xfffe;
    row[3] = 3;
    row[4] = 40000;
    row[5] = type;
    enabled = mask;
    calls = flag_count = 0;
    return func_8015616C(&actor, 37);
}

int main(void)
{
    unsigned rank;
    random_value = 17;
    assert(apply(0, 0, 0) == 22);
    assert(calls == 4 && flag_count == 1 && flag_query[0] == 0x81);
    assert((u16)D_8012693A == 40000 && D_801152B8 == 0);
    magic = -4;
    assert(apply(0xa000, 0, 32) == 3);
    assert(calls == 35 && modifier_input == 65532);
    assert((u16)D_8012693A == (u16)80000);

    first = INT32_MAX;
    second = 3;
    for (rank = 1; rank <= 5; ++rank) {
        u32 mask = (1u << rank) - 1;
        assert(apply(0x6000, 65535, mask) == 2);
        assert(calls == 12 && flag_count == 7 - rank);
        assert(flag_query[flag_count - 1] == 0x15 + rank);
        assert((u16)D_801152B8 == (u16)(65535 + rank));
    }
    assert(apply(0xc000, 40, 0) == 9);
    assert(calls == 125 && modifier_input == 2);
    assert((u16)D_801152B8 == 40 && flag_count == 6);
    return 0;
}
