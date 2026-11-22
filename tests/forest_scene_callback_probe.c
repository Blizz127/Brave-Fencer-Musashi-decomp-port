#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "forest_player_state.h"

_Static_assert(sizeof(ForestPlayerState) == 0x240, "actor size");
_Static_assert(offsetof(ForestPlayerState, field_ae) == 0xae, "stick axes");
_Static_assert(offsetof(ForestPlayerState, scene_callback) == 0x1f8, "callback slot");
static ForestPlayerState actor;
static unsigned inputs, calls;
static u32 expected;

void bfm_forest_player_call(u32 address, ForestPlayerState *player)
{
    assert(address == expected && player == &actor);
    if (inputs) {
        assert(player->field_aa == 0x48 && player->field_ac == 0x40 && player->field_ae == 0x5a12);
    } else {
        assert(player->field_aa == 0 && player->field_ac == 0 && player->field_ae == 0x8080);
    }
    player->field_aa = 0x100;
    player->scene_callback = 0;
    ++calls;
}

int main(void)
{
    unsigned flags, half;
    memset(&actor, 0, sizeof actor);
    func_8016F14C(&actor);
    assert(calls == 0 && actor.field_aa == 0);
    for (half = 0; half < 2; ++half) {
        expected = half ? 0x80023450u : 0x00023450u;
        for (flags = 0; flags < 128; ++flags) {
            actor.scene_callback = expected | (flags << 24);
            actor.field_aa = 0x48;
            actor.field_ac = 0x40;
            actor.field_ae = 0x5a12;
            inputs = !(flags & 1);
            func_8016F14C(&actor);
            assert(actor.field_aa == 0x100 && actor.scene_callback == 0);
        }
    }
    assert(calls == 256);
    actor.field_ac = 73;
    assert(func_80148634(&actor) == 0x8080);
    assert(actor.field_aa == 0 && actor.field_ac == 0 && actor.field_ae == 0x8080);
    return 0;
}
