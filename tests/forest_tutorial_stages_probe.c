#include <assert.h>
#include <stddef.h>
#include "forest_player_state.h"
#include "player_direction.h"

_Static_assert(offsetof(ForestPlayerState, scene_countdown) == 0x204, "scene countdown offset");
_Static_assert(offsetof(ForestPlayerState, scene_stage) == 0x214, "scene stage offset");
_Static_assert(offsetof(ForestPlayerState, step) == 0x234, "player step offset");

extern void func_8017E8A4(ForestPlayerState *);
extern void func_8017E924(ForestPlayerState *);
extern void func_8017E974(ForestPlayerState *);
extern void func_8017EA2C(ForestPlayerState *);

s32 D_801270D4;
static ForestPlayerState player;
static u32 flags;
static s16 heading;
static int direction_ready, counter_resets, close_calls;

u32 func_80172658(u8 *actor)
{
    assert(actor == (u8 *)&player);
    return flags & 1;
}

void func_80171B44(u8 *actor)
{
    assert(actor == (u8 *)&player);
    ++counter_resets;
}

void *bfm_forest_contact_pointer(u32 address, u32 width)
{
    assert(address == player.model + 0x12 && width == sizeof heading);
    return &heading;
}

s32 func_80171B4C(ScenePlayer *actor, s32 frames)
{
    assert((void *)actor == &player && frames == 8);
    return direction_ready;
}

u32 func_80172664(u8 *actor)
{
    assert(actor == (u8 *)&player);
    return flags & 2;
}

s32 func_8016F0E4(u8 *actor)
{
    assert(actor == (u8 *)&player && close_calls == 0);
    ++close_calls;
    return 0;
}

s32 func_80165770(void)
{
    assert(close_calls == 1);
    ++close_calls;
    return 12;
}

int main(void)
{
    player.model = 0x123400u;
    heading = 1024;
    for (player.kind = 25; player.kind <= 26; ++player.kind) {
        D_801270D4 = 30;
        func_8017E8A4(&player);
        assert(!player.scene_stage && heading == 1024);
    }
    player.kind = 0;
    func_8017E8A4(&player);
    assert(player.scene_stage == 4 && heading == 1024 && !counter_resets);

    player.scene_stage = 0;
    D_801270D4 = 29;
    func_8017E8A4(&player);
    assert(!player.scene_stage && heading == 1024);
    flags = 1;
    func_8017E8A4(&player);
    assert(player.scene_stage == 1 && !heading && counter_resets == 1);

    func_8017E924(&player);
    assert(player.scene_stage == 1 && !player.scene_countdown);
    direction_ready = 1;
    func_8017E924(&player);
    assert(player.scene_stage == 2 && player.scene_countdown == 32);
    assert(player.field_aa == 0xc040 && player.field_ac == 0xc040);

    player.field_aa = player.field_ac = 0;
    flags = 2;
    func_8017E974(&player);
    assert(player.scene_stage == 2 && player.field_aa == 0xc040 && player.field_ac == 0xc040);
    flags = 0;
    func_8017E974(&player);
    assert(player.scene_stage == 3);

    player.scene_stage = 4;
    func_8017EA2C(&player);
    assert(close_calls == 2);
    return 0;
}
