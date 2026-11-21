#include "player_direction.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static ScenePlayer player;
static SceneDirectionCamera camera;
static SceneDirectionLink link;

u32 D_801151D4;
static unsigned angle_calls;
static unsigned cleanup_calls;
static int replace_state;

s16 func_8004CFEC(s32 dx, s32 dz)
{
    assert(dx == 4 && dz == -7);
    assert(player.direction_mode == 0x41);
    ++angle_calls;
    return 0;
}

s32 func_80146D90(void *object)
{
    assert(object == &player);
    assert(player.direction_counter == 31);
    assert(player.direction_bits == 0x4000);
    ++cleanup_calls;
    if (replace_state)
        player.event_state = 250;
    return -55;
}

int main(void)
{
    unsigned i;

    _Static_assert(offsetof(ScenePlayer, event_state) == 0x214, "event state offset");
    _Static_assert(offsetof(ScenePlayer, direction_counter) == 0x20c, "counter offset");
    assert((uintptr_t)&camera <= UINT32_MAX && (uintptr_t)&link <= UINT32_MAX);
    D_801151D4 = (u32)(uintptr_t)&camera;
    player.link = (u32)(uintptr_t)&link;
    camera.target_x = 13;
    camera.x = 9;
    camera.target_z = 2;
    camera.z = 9;

    for (i = 0; i < 256; ++i) {
        player.event_state = (u8)i;
        player.direction_counter = 29;
        assert(func_8017E170(&player) == 0);
        assert(player.event_state == i && player.direction_counter == 30);
        assert(func_8017E170(&player) == (s32)(i + 1));
        assert(player.event_state == (u8)(i + 1) && player.direction_counter == 31);
    }

    player.direction_counter = 0;
    player.event_state = 7;
    for (i = 0; i < 30; ++i)
        assert(func_8017E170(&player) == 0 && player.event_state == 7);
    assert(func_8017E170(&player) == 8 && player.event_state == 8);

    /* The cleanup provider can update state before the callback increments it. */
    replace_state = 1;
    player.direction_counter = 30;
    player.event_state = 9;
    assert(func_8017E170(&player) == 251 && player.event_state == 251);
    assert(angle_calls == 544 && cleanup_calls == 258);
    return 0;
}
