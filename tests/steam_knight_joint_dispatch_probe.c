#include "steam_knight_hit.h"
#include <assert.h>
#include <string.h>

void func_80189AF4(SteamKnightPart *);
s32 func_8018B300(void);
u8 D_801E44F0[8];
static SteamKnightPart effect;
static u32 table_address, target, delivered;
static u8 unaligned_bytes[8];
static unsigned resolutions, dispatches;

void *bfm_tower_spawn_pointer(u32 address)
{
    assert(address == table_address);
    ++resolutions;
    return unaligned_bytes + 1;
}

void bfm_tower_spawn_setup(u32 address, void *actor)
{
    assert(actor == &effect);
    assert(address == target);
    ++dispatches;
    delivered = address;
}

int main(void)
{
    unsigned i, occupancy;
    for (occupancy = 0; occupancy <= 255; ++occupancy) {
        u8 initial[8];
        int expected = -1;
        for (i = 0; i < 8; ++i) {
            initial[i] = occupancy & (1u << i) ? (u8)(255 - 17*i) : 0;
            if (!initial[i] && expected < 0)
                expected = i;
        }
        memcpy(D_801E44F0, initial, sizeof initial);
        assert(func_8018B300() == expected);
        if (expected >= 0) initial[expected] = 1;
        assert(!memcmp(D_801E44F0, initial, sizeof initial));
    }
    memset(&effect, 0xb9, sizeof effect);
    for (i = 0; i <= 65535; ++i) {
        effect.state = (u16)i;
        table_address = 0x8019660cu + 4u * i;
        target = 0x80100000u ^ (i * 0x190d75u);
        memcpy(unaligned_bytes + 1, &target, sizeof target);
        func_80189AF4(&effect);
        assert(delivered == target);
    }
    assert(resolutions == 65536 && dispatches == 65536);
    return 0;
}
