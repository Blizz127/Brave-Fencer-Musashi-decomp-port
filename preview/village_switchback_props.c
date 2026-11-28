/* SPDX-License-Identifier: MIT */
#include "village_switchback_props.h"
#include "village_mine_corridor_387.h"

#include <stdlib.h>

struct BfmVillageSwitchbackProps {
    BfmVillageGround *ground;
    BfmVillageMineCorridor387 *creature;
    BfmMineCreatureShadow shadow;
};

void bfm_village_switchback_props_free(BfmVillageSwitchbackProps *props)
{
    if (props) {
        bfm_village_mine_corridor_387_free(props->creature);
        bfm_village_ground_free(props->ground);
        free(props);
    }
}

BfmVillageSwitchbackProps *bfm_village_switchback_props_open(const uint8_t *scene, size_t bytes,
    const int16_t hero[3], unsigned world_tick)
{
    if (!scene || !hero) return NULL;
    BfmVillageSwitchbackProps *props = calloc(1, sizeof *props);
    if (!props) return NULL;
    props->ground = bfm_village_ground_open(scene, bytes, UINT32_C(0x80184870));
    props->creature = bfm_village_mine_corridor_387_open(scene, bytes, props->ground);
    if (props->creature) {
        bfm_village_mine_corridor_387_enter(props->creature, hero, world_tick);
        if (bfm_village_mine_corridor_387_shadow_start(props->creature,
                BFM_MINE_WEST_CREATURE, &props->shadow)) return props;
    }
    bfm_village_switchback_props_free(props);
    return NULL;
}

int bfm_village_switchback_props_step(BfmVillageSwitchbackProps *props, const int16_t hero[3],
    unsigned world_tick, BfmGameplayRun *run, int suppress_tracking)
{
    return bfm_village_switchback_props_step_target(props, hero, suppress_tracking ? NULL : hero,
                                                  world_tick, run, NULL);
}

int bfm_village_switchback_props_step_target(BfmVillageSwitchbackProps *props, const int16_t hero[3],
    const int16_t target[3], unsigned world_tick, BfmGameplayRun *run, BfmMineLungeHit *hit)
{
    if (!props || !hero || !run) return 0;
    if (hit) *hit = (BfmMineLungeHit){0};
    /* The fixed placement has its own actor slot. Its shadow child runs
     * immediately afterwards and follows the parent's new ground position. */
    bfm_village_mine_corridor_387_step(props->creature, hero, target, run, world_tick, hit);
    if (props->shadow.active)
        bfm_village_mine_corridor_387_shadow_step(props->creature, &props->shadow);
    return 1;
}

int bfm_village_switchback_props_draw(void *context, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    BfmVillageSwitchbackProps *props = context;
    return props && bfm_village_mine_corridor_387_draw_body(props->creature, BFM_MINE_WEST_CREATURE,
        figure, view) && bfm_village_mine_corridor_387_shadow_draw(props->creature,
        &props->shadow, figure, view);
}
