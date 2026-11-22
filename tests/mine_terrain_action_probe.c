#include "mine_terrain_action.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(MineActionPoint) == 8, "terrain point size");
_Static_assert(offsetof(MineTerrainActor, position) == 4, "player position");
_Static_assert(offsetof(MineTerrainActor, previous) == 0x88, "previous position");
_Static_assert(offsetof(MineTerrainActor, action) == 0x16c, "pending action");
_Static_assert(offsetof(MineTerrainActor, surface_class) == 0x16e, "surface class");

SceneProbeVector D_801152A8, D_801152B0;
s32 D_801152BC;
static int queries, flag_checks;
static s32 terrain_result, flag_result;
static u16 reported_surface;
static MineTerrainActor *subject;

s32 func_80133784(s16 mode, MineActionPoint *from, MineActionPoint *to)
{
    assert(++queries == 1 && mode == 1 && flag_checks == 0);
    assert(from->x == subject->previous[0] && from->y == subject->previous[1] && from->z == subject->previous[2]);
    assert(to->x == subject->position[1] && to->y == subject->position[3] && to->z == subject->position[5]);
    to->x = 148;
    to->y = (u16)-72;
    to->z = 0x8301;
    to->surface.flags = reported_surface;
    return terrain_result;
}

s32 func_80029178(u32 flag)
{
    assert(++flag_checks == 1 && queries == 1 && flag == 33);
    assert(subject->action == 29 && subject->surface_class == reported_surface >> 13);
    return flag_result;
}

static void check(s32 query, u16 surface, u16 prior, s32 game_flag)
{
    MineTerrainActor actor, expected;
    SceneProbeVector original_normal = {12, -23, 45, 11};
    SceneProbeVector retained_normal = {-24, 5, 17, 31};
    s32 result = query;
    memset(&actor, 0x46, sizeof(actor));
    actor.position[1] = 323; actor.position[3] = (u16)-24; actor.position[5] = 0x801d;
    actor.previous[0] = 322; actor.previous[1] = (u16)-25; actor.previous[2] = 0x801c;
    actor.action = prior;
    actor.surface_class = 0x7331;
    expected = actor;
    if (surface & 255) expected.action = surface & 255;
    if (query) {
        expected.position[1] = 148;
        expected.position[3] = (u16)-72;
        expected.position[5] = 0x8301;
        expected.surface_class = surface >> 13;
    }
    int tests_flag = query != 0 && expected.action == 29;
    int saves_normal = tests_flag && (u8)game_flag == 0;
    if (saves_normal) result = (result & ~0x2000) | 0x4000;
    D_801152A8 = original_normal;
    D_801152B0 = retained_normal;
    D_801152BC = 7;
    subject = &actor;
    reported_surface = surface;
    terrain_result = query;
    flag_result = game_flag;
    queries = flag_checks = 0;
    assert(func_8014FA70(&actor) == result);
    assert(queries == 1 && flag_checks == tests_flag);
    assert(memcmp(&actor, &expected, sizeof(actor)) == 0);
    assert(memcmp(&D_801152A8, &original_normal, sizeof(original_normal)) == 0);
    assert(memcmp(&D_801152B0, saves_normal ? &original_normal : &retained_normal,
                  sizeof(retained_normal)) == 0);
    assert(D_801152BC == (saves_normal ? 1 : 7));
}

int main(void)
{
    const s32 results[] = {0, 1, 0x2011, -1};
    const u16 surfaces[] = {0, 9, 0x401d, 0xe000, 0xa09d};
    const u16 prior[] = {13, 29};
    const s32 flags[] = {0, 1, 256, -1};
    for (size_t i = 0; i < sizeof(results)/sizeof(results[0]); ++i)
        for (size_t j = 0; j < sizeof(surfaces)/sizeof(surfaces[0]); ++j)
            for (size_t k = 0; k < sizeof(prior)/sizeof(prior[0]); ++k)
                for (size_t l = 0; l < sizeof(flags)/sizeof(flags[0]); ++l)
                    check(results[i], surfaces[j], prior[k], flags[l]);
    return 0;
}
