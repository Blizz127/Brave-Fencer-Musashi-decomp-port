#include "fade_setup.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

u8 D_80126B58[34];
static u32 records[3];
static u32 *selected;
static u32 flags;
static u16 new_id;
static unsigned calls;
static s32 ef_value, a4_value, b3_value;

s32 func_8016EF78(u32 *record, u32 mask)
{
    assert(calls++ == 0 && record == selected && mask == flags);
    D_80126B58[0] = (u8)new_id;
    D_80126B58[1] = (u8)(new_id >> 8);
    return ef_value;
}

s32 func_801599A4(u8 *actor)
{
    assert(calls++ == 1 && actor == D_80126B58);
    assert((u16)(actor[0] | ((u16)actor[1] << 8)) == new_id);
    actor[0] = 25;
    actor[1] = 0;
    return a4_value;
}

s32 func_80159B3C(u8 *actor)
{
    assert(calls++ == 2 && actor == D_80126B58);
    assert(actor[0] == 25 && actor[1] == 0);
    return b3_value;
}

int main(void)
{
    const s32 values[] = {0, -1, INT_MIN, INT_MAX, 0x13579bd};
    const u8 sentinel = 0xc7;
    unsigned id, k, index, cases = 0;
    for (id = 0; id <= 65535; ++id) {
        for (k = 0; k < sizeof(values) / sizeof(values[0]); ++k) {
            int excluded = id == 25 || id == 26;
            memset(D_80126B58, sentinel, sizeof(D_80126B58));
            D_80126B58[0] = excluded ? 0 : 26;
            D_80126B58[1] = 0;
            selected = k < 3 ? &records[k] : NULL;
            flags = id * 0x10001u ^ (0x13579bdu * k);
            calls = 0;
            new_id = (u16)id;
            ef_value = values[(k + 1) % 5];
            a4_value = values[(k + 2) % 5];
            b3_value = values[k];
            assert(func_8016EE40(selected, flags) == (excluded ? 1 : b3_value));
            assert(calls == (excluded ? 1u : 3u));
            assert(D_80126B58[0] == (excluded ? id : 25));
            assert(D_80126B58[1] == 0);
            for (index = 2; index < sizeof(D_80126B58); ++index)
                assert(D_80126B58[index] == sentinel);
            ++cases;
        }
    }
    printf("%u cases, all 65,536 actor IDs, exact return/forwarding/order\n", cases);
    return 0;
}
