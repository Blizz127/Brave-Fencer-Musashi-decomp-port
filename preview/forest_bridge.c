/* SPDX-License-Identifier: MIT */
#include "forest_bridge.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>

enum { UPRIGHT_TARGET = 0x8000, GUNSHOT_HIT = 0x100001, LANDED_FLAGS = 0xcc00 };

int bfm_forest_bridge_load(BfmForestBridge *bridge, const BfmForestField *field)
{
    const uint8_t *thresholds = bfm_forest_room_bytes(field, UINT32_C(0x8018a8fc),
                                                       2 * BFM_FOREST_BRIDGE_TARGET_COUNT);
    if (!thresholds || !bfm_forest_bridge_floor_load(field, &bridge->floor)) return 0;
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        int threshold = (int16_t)u16le(thresholds + 2 * role);
        if (threshold <= 0) return 0;
        bridge->logs[role].threshold = threshold;
    }
    bridge->targets = bfm_forest_bridge_targets_open(field);
    return bridge->targets != NULL;
}

void bfm_forest_bridge_approach(BfmForestBridge *bridge, int z)
{
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        BfmForestLog *log = &bridge->logs[role];
        const BfmForestActor *actor = bfm_forest_bridge_targets_actor(bridge->targets, role);
        if (log->state == BFM_LOG_UNPLACED && actor &&
            abs(z - actor->root->translation[2]) < 768) {
            log->state = BFM_LOG_STANDING;
            log->flags = UPRIGHT_TARGET;
        }
    }
}

unsigned bfm_forest_bridge_contact(BfmForestBridge *bridge, const int16_t from[3],
                                   const int16_t to[3], int16_t hit[3], int16_t normal[3])
{
    unsigned eligible = 0;
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        const BfmForestLog *log = &bridge->logs[role];
        if (log->state == BFM_LOG_STANDING && (log->flags & UPRIGHT_TARGET))
            eligible |= 1u << role;
    }
    unsigned contacts = bfm_forest_bridge_targets_impact(bridge->targets, eligible, from, to, hit, normal);
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role)
        if (contacts & (1u << role))
            bridge->logs[role].flags |= GUNSHOT_HIT;
    return contacts;
}

/* The model sees the integrated pitch before the hinge is clamped at each
 * bounce. The final impact also fixes the visible model at a quarter turn. */
void bfm_forest_log_fall(BfmForestLog *log)
{
    log->velocity += 4 << 16;
    log->angle += log->velocity;
    log->pitch = log->angle >> 16;
    if (log->pitch >= 1024) {
        log->flags |= LANDED_FLAGS;
        log->angle = 1024 << 16;
        if (log->bounces == 2) {
            log->state = BFM_LOG_SETTLED;
            log->pitch = 1024;
        } else {
            log->velocity = -(log->velocity >> 1);
            ++log->bounces;
        }
    }
}

unsigned bfm_forest_bridge_advance(BfmForestBridge *bridge, unsigned lesson_phase)
{
    if (bridge->phase < 3) bridge->phase = lesson_phase;
    if (bridge->counter && bridge->phase >= 2) {
        bridge->phase = 3;
        if (bridge->counter < UINT_MAX) ++bridge->counter;
    }
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        BfmForestLog *log = &bridge->logs[role];
        if (log->state == BFM_LOG_FALLING) {
            bfm_forest_log_fall(log);
            bfm_forest_bridge_targets_pitch(bridge->targets, role, (unsigned)log->pitch);
        } else if (log->state == BFM_LOG_STANDING) {
            if (bridge->phase == 0 && (log->flags & 1u)) {
                bridge->phase = 3;
                if (!bridge->counter) bridge->counter = 1;
                log->flags &= ~UINT32_C(0xffff);
            }
            if (bridge->phase < 2) continue;
            if (!bridge->counter && (log->flags & 1u)) bridge->counter = 1;
            if (bridge->counter < (unsigned)log->threshold) continue;
            log->state = BFM_LOG_FALLING;
            log->flags &= ~UINT32_C(0x8c00);
            log->velocity = 16 << 16;
        }
    }
    return bridge->phase;
}

int bfm_forest_bridge_support(const BfmForestBridge *bridge, BfmForestBridgeProbe query,
                              const int16_t from[3], const int16_t to[3], BfmForestContact *floor)
{
    int found = 0;
    for (unsigned role = 0; role < BFM_FOREST_BRIDGE_TARGET_COUNT; ++role) {
        const BfmForestLog *log = &bridge->logs[role];
        if (!(log->flags & 0x0400)) continue;
        const BfmForestActor *actor = bfm_forest_bridge_targets_actor(bridge->targets, role);
        BfmForestBridgeSurface contact;
        if (!bfm_forest_bridge_floor_contact(&bridge->floor, actor->root, log->state, log->flags,
                                             query, from, to, &contact)) continue;
        if (!found || contact.height < floor->height) {
            *floor = (BfmForestContact){.height = contact.height,
                .normal = {contact.normal[0], contact.normal[1], contact.normal[2]},
                .bridge = 1};
            found = 1;
        }
    }
    return found;
}
