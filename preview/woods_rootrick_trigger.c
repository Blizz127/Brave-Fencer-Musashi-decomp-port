/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_trigger.h"
#include "gameplay_run.h"
#include "../pc_port/forest_player_bytes.h"

int bfm_woods_rootrick_trigger_load(BfmWoodsRootrickTrigger *out, const uint8_t *scene, size_t bytes)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t room = UINT32_C(0x801961e8) - base;
    const uint32_t opening = UINT32_C(0x80195198);
    if (!out || !scene || bytes < room + 0xbc) return 0;
    uint32_t first = u32le(scene + room + 0xb4), end = u32le(scene + room + 0xb8);
    if (first < base || end <= first || end - base > bytes || (end - first) % 20) return 0;

    BfmWoodsRootrickTrigger next = {0};
    for (size_t pos = first - base; pos < end - base; pos += 20) {
        unsigned kind = u16le(scene + pos + 6);
        if (!kind) {
            if (!next.opening_address) return 0;
            *out = next;
            return 1;
        }
        if (kind != 128) continue;
        if (next.opening_address) return 0;
        for (unsigned axis = 0; axis < 3; ++axis) next.actor_xyz[axis] = s16le(scene + pos + 2 * axis);
        next.opening_address = opening;
    }
    return 0;
}

int bfm_woods_rootrick_trigger_step(BfmWoodsRootrickTrigger *trigger, const BfmGameplayRun *run,
                                    const int16_t feet[3])
{
    if (!trigger || !trigger->opening_address || !run || !feet) return 0;
    if (trigger->state == BFM_ROOTRICK_ENTERING) {
        trigger->state = run->banks.variables[205] && !bfm_gameplay_story_flag(run, 158)
                       ? BFM_ROOTRICK_WAITING : BFM_ROOTRICK_INACTIVE;
        return 0;
    }
    if (trigger->state != BFM_ROOTRICK_WAITING) return 0;
    /* The bend is a world-space region, separate from the actor's position
     * on the cliff. Y increases downwards in the field coordinate system. */
    if (feet[0] < -605 || feet[0] > -385 || feet[1] < -322 ||
        feet[2] < 963 || feet[2] > 1759) return 0;
    trigger->state = BFM_ROOTRICK_STARTED;
    return 1;
}
