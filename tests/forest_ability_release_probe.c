#include <assert.h>
#include "forest_player_state.h"

extern void func_80165AC8(char *player);
u8 D_80078EC1;
static ForestPlayerState player;
static u32 slot, calls, lookups;

u32 bfm_forest_player_table_word(u32 address, u32 index)
{
    assert(address == 0x80189974u);
    assert(index == (D_80078EC1 & 127u));
    slot = index;
    ++lookups;
    return 0x81230000u + index * 4u;
}

void bfm_forest_player_call(u32 address, ForestPlayerState *actor)
{
    assert(actor == &player);
    assert(address == 0x81230000u + slot * 4u);
    assert(lookups == calls + 1);
    ++calls;
}

int main(void)
{
    for (unsigned selection = 0; selection < 256; ++selection) {
        D_80078EC1 = (u8)selection;
        func_80165AC8((char *)&player);
        assert(calls == selection + 1);
    }
    return 0;
}
