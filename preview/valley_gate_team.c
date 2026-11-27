/* SPDX-License-Identifier: MIT */
#include "valley_gate_team.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

static const uint32_t origin = UINT32_C(0x80128158);
enum {
    ROOM = 0x80186620u - 0x80128158u,
    SCRIPT = 0x8018634cu - 0x80128158u, SCRIPT_END = 0x80186434u - 0x80128158u,
    TEXT = 0x80186748u - 0x80128158u, TEXT_END = 0x801868c4u - 0x80128158u
};
enum { RETIRED, ARMED, WAIT_PLAYER, RUNNING, COMPLETE };

int bfm_valley_gate_team_init(BfmValleyGateTeam *team, const uint8_t *scene, size_t bytes,
                              const BfmSaveRecord *save)
{
    if (!team || !scene || !save || bytes < TEXT_END) return 0;
    uint32_t first = u32le(scene + ROOM + 0xb4), end = u32le(scene + ROOM + 0xb8);
    if (first < origin || end < first || end-origin > bytes || (end-first)%20) return 0;
    const uint8_t *actor = scene + (first-origin), *limit = scene + (end-origin);
    while (actor < limit && u16le(actor+6) != 305) actor += 20;
    if (actor == limit) return 0;
    uint32_t story = u32le(save->progress+8);
    *team = (BfmValleyGateTeam){.scene=scene, .cursor=scene+SCRIPT, .end=scene+SCRIPT_END,
        .members_present=story == 150, .gate_open_on_entry=story >= 160,
        .phase=story == 150 ? ARMED : RETIRED};
    return 1;
}

int bfm_valley_gate_team_active(const BfmValleyGateTeam *team)
{
    return team && team->phase >= WAIT_PLAYER;
}

int bfm_valley_gate_team_message_closed(BfmValleyGateTeam *team)
{
    if (!team || !team->window) return 0;
    team->window = 0;
    return 1;
}

static void receive_signal(BfmValleyGateTeam *team)
{
    /* The actor receives last update's script signal before advancing the VM. */
    unsigned slot = team->pending_signal;
    if (slot == BFM_VALLEY_GATE_MEMBER_COUNT+1) {
        memset(team->member,BFM_VALLEY_GATE_WALK_TO_SWITCH,sizeof team->member);
    } else if (slot) {
        team->member[slot-1] ^= 1;
    }
    team->pending_signal = 0;
}

BfmValleyGateResult bfm_valley_gate_team_step(BfmValleyGateTeam *team, int hero_z,
    int player_action_ready, BfmSaveRecord *save, BfmGameplayRun *run, BfmForestMessage **opened)
{
    if (!team || !save || !opened) return BFM_VALLEY_GATE_ERROR;
    team->face_point = 0;
    if (team->phase == RETIRED) return BFM_VALLEY_GATE_FREE;
    if (team->phase == ARMED) {
        if (hero_z < 2048) return BFM_VALLEY_GATE_FREE;
        team->phase = WAIT_PLAYER;
        return BFM_VALLEY_GATE_LOCKED;
    }
    receive_signal(team);
    if (team->phase == COMPLETE) {
        /* Only the parent's completed-script poll promotes the visit. The
         * Soldier's physical landing has its own gate and sound events. */
        memset(save->progress+8,0,4);
        save->progress[8] = 160;
        if (run && run->has_player) memcpy(run->player.record+8,save->progress+8,4);
        team->phase = RETIRED;
        return BFM_VALLEY_GATE_RELEASED;
    }
    if (team->phase == WAIT_PLAYER && !player_action_ready) return BFM_VALLEY_GATE_LOCKED;
    team->phase = RUNNING;
    if (team->window || (team->delay && --team->delay)) return BFM_VALLEY_GATE_LOCKED;
    while (team->cursor < team->end) {
        const uint8_t *row = team->cursor;
        unsigned op = u32le(row);
        uint32_t arg = u32le(row+4);
        team->cursor += 8;
        switch (op) {
        case 0: case 1: {
            if (*opened || arg < origin+TEXT || arg >= origin+TEXT_END) return BFM_VALLEY_GATE_ERROR;
            size_t offset = arg-origin;
            if (team->scene[offset] != 5) return BFM_VALLEY_GATE_ERROR;
            *opened = bfm_field_message_borrow_flags(team->scene+offset,TEXT_END-offset,save->flags);
            if (!*opened) return BFM_VALLEY_GATE_ERROR;
            team->window = 1;
            team->window_offset = op ? 0 : team->actor_offset;
            team->window_right = op ? 0 : team->actor_right;
            return BFM_VALLEY_GATE_LOCKED;
        }
        case 2:
            if (team->cursor != team->end) return BFM_VALLEY_GATE_ERROR;
            team->phase = COMPLETE;
            return BFM_VALLEY_GATE_LOCKED;
        case 9:
            team->face_point = 1;
            team->face_x = s16le(row+4);
            team->face_z = s16le(row+6);
            break;
        case 15: case 63:
            if (!arg) return BFM_VALLEY_GATE_ERROR;
            team->delay = arg;
            return BFM_VALLEY_GATE_LOCKED;
        case 46:
            if (!arg || arg > BFM_VALLEY_GATE_MEMBER_COUNT+1) return BFM_VALLEY_GATE_ERROR;
            team->pending_signal = arg;
            break;
        case 55:
            team->actor_offset = s16le(row+4);
            team->actor_right = u16le(row+6) != 0;
            break;
        case 64:
            team->phase = WAIT_PLAYER;
            return BFM_VALLEY_GATE_LOCKED;
        default: return BFM_VALLEY_GATE_ERROR;
        }
    }
    return BFM_VALLEY_GATE_ERROR;
}
