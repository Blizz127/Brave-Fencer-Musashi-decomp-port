#include <assert.h>
#include <stddef.h>
#include "forest_player_state.h"

extern void func_8017E868(ForestPlayerState *player);
extern void func_8017E9C0(ForestPlayerState *player);

_Static_assert(offsetof(ForestPlayerState, scene_stage) == 0x214, "scene stage");
_Static_assert(offsetof(ForestPlayerState, step) == 0x234, "player step");

static ForestPlayerState player;
static s16 model_yaw;
static unsigned calls, lookups;
static int run_stage;

u32 bfm_forest_player_table_word(u32 address, u32 index)
{
    assert(address == 0x8018a79cu && index == player.scene_stage);
    ++lookups;
    return 0x81000000u + 4u * index;
}

void bfm_forest_player_call(u32 address, ForestPlayerState *actor)
{
    assert(actor == &player);
    assert(address == 0x81000000u + 4u * player.scene_stage);
    assert(lookups == calls + 1);
    ++calls;
    if (run_stage) {
        assert(actor->scene_stage == 3);
        func_8017E9C0(actor);
    }
}

void *bfm_forest_contact_pointer(u32 address, u32 size)
{
    assert(address == player.model + 0x12 && size == sizeof(model_yaw));
    return &model_yaw;
}

int main(void)
{
    for (unsigned i = 0; i < 256; ++i) {
        player.scene_stage = (u8)i;
        func_8017E868(&player);
    }
    assert(calls == 256 && lookups == calls);

    player.model = 0x81100300u;
    player.scene_stage = 3;
    run_stage = 1;
    model_yaw = 2040;
    const s16 expected[] = {2042, 2043, 2044, 2045, 2046, 2047, 2048, 2048};
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        func_8017E868(&player);
        assert(model_yaw == expected[i]);
        assert(player.scene_stage == (i == 7 ? 4 : 3));
    }
    player.scene_stage = 3;
    model_yaw = 2049;
    func_8017E868(&player);
    assert(model_yaw == 2048 && player.scene_stage == 3);
    return 0;
}
