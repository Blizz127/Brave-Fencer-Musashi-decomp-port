#include "bincho_burst.h"

ForestFieldPhase func_8017CD18;

void func_8017CD18(void *actor_slot)
{
    BinchoBurstActor *actor = actor_slot;
    /* The compiled actor phase reserves 16 unreferenced frame bytes. */
    u32 reserved_frame[4];
    BinchoReleaseConfig *config = actor->config;
    u16 *outer_effect = actor->outer_effect;
    BinchoBurstSprite *glow = actor->glow;

    if (config->load_effect && !actor->effect_handle) {
        actor->effect_handle = func_80128CFC(config->effect_id);
    }
    glow->width = glow->height += 32;
    if (--actor->timer != -1) {
        func_8017C294(actor, func_8005C49C() & 1);
    } else {
        D_80115214 = 4;
        --D_801152BA;
        func_8002D4C8(0x62e, 0);
        *outer_effect = 0;
        actor->timer = 40;
        ++actor->phase;
    }
}
