/* SPDX-License-Identifier: MIT */
#include "forest_speech_actor.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

/* The registered doorway names these mounted animation resources. */
static uint32_t script_address(BfmForestTalkPhase phase)
{
    switch (phase) {
    case BFM_FOREST_TALK_START: return UINT32_C(0x800d5a34);
    case BFM_FOREST_TALK_FIRST: return UINT32_C(0x8018a62c);
    case BFM_FOREST_TALK_SECOND: return UINT32_C(0x8018a69c);
    case BFM_FOREST_TALK_END_GESTURE: return UINT32_C(0x8018a6e4);
    case BFM_FOREST_TALK_RELEASED: return UINT32_C(0x800d4854);
    }
    return 0;
}

static const uint8_t *resource(const BfmForestSpeechActor *actor, uint32_t address,
                               uint8_t *local, size_t length)
{
    const uint8_t *room = bfm_forest_room_bytes(actor->field, address, length);
    if (room)
        return room;
    return forest_player_archive_read(&actor->player, address, local, length) ? local : NULL;
}

static int operand(BfmForestSpeechActor *actor, uint32_t *word)
{
    if (actor->cursor >= 128 || actor->script > UINT32_MAX - 4 * actor->cursor)
        return 0;
    uint8_t local[4];
    const uint8_t *p = resource(actor, actor->script + 4 * actor->cursor, local, sizeof local);
    if (!p)
        return 0;
    ++actor->cursor;
    *word = u32le(p);
    return 1;
}

/* The monitored motion programs all display one pose per actor update.
 * A budget bounds malformed command loops that display no pose. */
static int advance(BfmForestSpeechActor *actor, BfmForestPose *pose, int stop_at_loop)
{
    if (actor->complete)
        return 1;
    for (unsigned i = 0; i < 32; ++i) {
        uint32_t cmd, value;
        if (!operand(actor, &cmd))
            return 0;
        switch (cmd) {
        case 0:
            actor->complete = 1;
            return actor->frame != 0;
        case 1:
            actor->cursor = 0;
            if (stop_at_loop) {
                actor->complete = 1;
                return actor->frame != 0;
            }
            break;
        case 4: /* Preserve the current mounted player mesh. */
            if (!operand(actor, &value) || value != UINT32_C(0x800de2a4))
                return 0;
            break;
        case 6:
            if (!operand(actor, &value) || value >= 128)
                return 0;
            actor->cursor = (unsigned)value;
            break;
        case 7: /* An action marker shares the following body key. */
            break;
        case 15: /* Facial texture sequence selected alongside the body. */
            if (!operand(actor, &value) || value > 7)
                return 0;
            actor->face_slot = value;
            break;
        case 16:
            actor->visible |= UINT32_C(1) << 22;
            actor->visible &= ~(UINT32_C(1) << 21);
            break;
        case 17:
            /* The two blade mesh parts exchange visibility in this gesture. */
            actor->visible |= UINT32_C(1) << 21;
            actor->visible &= ~(UINT32_C(1) << 22);
            break;
        case 19:
            actor->visible = actor->entry_visible;
            break;
        default: {
            if (cmd < 32 || (cmd & 3) || cmd > UINT32_MAX - 16)
                return 0;
            uint32_t frame = cmd + 16;
            if (actor->frame != frame) {
                uint8_t bytes[256];
                const uint8_t *p = resource(actor, cmd, bytes, sizeof bytes);
                if (!p || !bfm_forest_pose_decode(p + 16, sizeof bytes - 16, pose))
                    return 0;
                memcpy(actor->blade_anchors, p, sizeof actor->blade_anchors);
                actor->frame = frame;
            }
            return 1;
        }
        }
    }
    return 0;
}

static int select_motion(BfmForestSpeechActor *actor, uint32_t script, BfmForestPose *pose)
{
    actor->script = script;
    actor->cursor = 0;
    actor->complete = 0;
    /* Selecting a motion displays its first pose. The scripted actor then
     * receives its normal animation service later in the same update. */
    return advance(actor, pose, 0);
}

int bfm_forest_speech_motion_tick(BfmForestSpeechActor *actor, uint32_t script,
                                  BfmForestPose *pose)
{
    if (!actor || !actor->field || !pose ||
        (script && !select_motion(actor, script, pose))) return 0;
    return advance(actor, pose, 0);
}

static int select_phase(BfmForestSpeechActor *actor, BfmForestTalkPhase phase,
                        BfmForestPose *pose)
{
    actor->phase = phase;
    return select_motion(actor, script_address(phase), pose);
}

int bfm_forest_speech_actor_begin(BfmForestSpeechActor *actor, const BfmForestField *field,
                                  const MusashiDiscMedia *disc, uint32_t parts,
                                  BfmForestPose *pose)
{
    if (!actor || !field || !disc || !pose)
        return 0;
    BfmForestSpeechActor fresh = {.field = field, .visible = parts, .entry_visible = parts};
    if (!forest_player_archive_open(disc, &fresh.player) ||
        !select_phase(&fresh, BFM_FOREST_TALK_START, pose) || !advance(&fresh, pose, 0))
        return 0;
    *actor = fresh;
    return 1;
}

int bfm_forest_speech_actor_tick(BfmForestSpeechActor *actor, int first, int second,
                                 int closed, BfmForestPose *pose)
{
    if (!actor || !actor->field || !pose)
        return 0;
    if (closed)
        return select_phase(actor, BFM_FOREST_TALK_RELEASED, pose);
    if (first && !select_phase(actor, BFM_FOREST_TALK_FIRST, pose))
        return 0;
    if (second && !select_phase(actor, BFM_FOREST_TALK_SECOND, pose))
        return 0;
    if (actor->phase == BFM_FOREST_TALK_SECOND && actor->complete &&
        !select_phase(actor, BFM_FOREST_TALK_END_GESTURE, pose))
        return 0;
    return advance(actor, pose, 0);
}

/* These are motion slots in the room's mounted player animation table. */
enum { BASE_STANCE, SHIFT_STANCE, DRAW_BLADE, BLADE_READY, RETURN_TO_STAND, RUN_TO_STAND = 9 };
enum { RESTING, SHIFTING, RESTING_AFTER_SHIFT, DRAWING, READY, COASTING };

static int standing_motion(BfmForestSpeechActor *actor, unsigned slot, BfmForestPose *pose)
{
    BfmForestOpeningData data;
    uint8_t bytes[4];
    if (!bfm_forest_opening_data(actor->field, &data) ||
        data.animation_table > UINT32_MAX - 4 * slot ||
        !forest_player_archive_read(&actor->player, data.animation_table + 4 * slot,
                                    bytes, sizeof bytes))
        return 0;
    return select_motion(actor, u32le(bytes), pose);
}

int bfm_forest_clearing_pose_tick(BfmForestSpeechActor *actor, BfmForestClearingEvent event,
                                  BfmForestPose *pose)
{
    if (!actor || !actor->field || !pose) return 0;
    switch (event) {
    case BFM_FOREST_CLEARING_CLIFF:
        if (!select_motion(actor, UINT32_C(0x8018a59c), pose)) return 0;
        break;
    case BFM_FOREST_CLEARING_RECOVER:
        if (!select_motion(actor, UINT32_C(0x8018a5e4), pose)) return 0;
        break;
    case BFM_FOREST_CLEARING_FACE_SOLDIER:
        /* On releasing the held gesture the standard player action selects
         * its first stance frame at the end of this update. */
        return standing_motion(actor, BASE_STANCE, pose);
    case BFM_FOREST_CLEARING_CONTINUE:
        break;
    default:
        return 0;
    }
    return advance(actor, pose, 0);
}

/* The ordinary standing action (SC01 member 77, 80159C84) makes its
 * decisions at the ends of mounted motions, including their loop marks. */
int bfm_forest_standing_tick(BfmForestSpeechActor *actor, BfmForestStandingEvent event,
                             const ForestPlayerState *player, BfmForestPose *pose)
{
    if (!actor || !actor->field || !player || !pose ||
        actor->phase != BFM_FOREST_TALK_RELEASED || (player->flags & 0x200))
        return 0;
    if (event != BFM_FOREST_STANDING_CONTINUE) {
        int leaving_run = event == BFM_FOREST_STANDING_RUN_STOP;
        actor->standing_step = leaving_run ? COASTING : RESTING;
        actor->standing_cycles = 0;
        /* The shared pose may have changed while the player was moving. */
        actor->frame = 0;
        return standing_motion(actor, leaving_run ? RUN_TO_STAND : RETURN_TO_STAND, pose);
    }
    if (!advance(actor, pose, 1))
        return 0;
    if (!actor->complete)
        return 1;

    unsigned slot = BASE_STANCE;
    if (actor->standing_step == COASTING)
        actor->standing_step = RESTING;
    if (player->scene_callback & UINT32_C(0x80ffffff)) {
        actor->standing_step = RESTING;
        actor->standing_cycles = 0;
    }
    switch (actor->standing_step) {
    case RESTING:
        if (++actor->standing_cycles > 4) {
            actor->standing_cycles = 0;
            actor->standing_step = SHIFTING;
            slot = SHIFT_STANCE;
        }
        break;
    case SHIFTING:
        actor->standing_step = RESTING_AFTER_SHIFT;
        break;
    case RESTING_AFTER_SHIFT:
        if (++actor->standing_cycles > 2) {
            actor->standing_cycles = 0;
            if (player->flags & 4) {
                actor->standing_step = RESTING;
            } else {
                actor->standing_step = DRAWING;
                slot = DRAW_BLADE;
            }
        }
        break;
    case DRAWING:
        actor->standing_step = READY;
        slot = BLADE_READY;
        break;
    case READY:
        slot = BLADE_READY;
        break;
    default:
        return 0;
    }
    if (!standing_motion(actor, slot, pose))
        return 0;
    if (slot == BLADE_READY) {
        actor->visible |= UINT32_C(1) << 21;
        actor->visible &= ~(UINT32_C(1) << 22);
    }
    return 1;
}

int32_t bfm_forest_standing_forward_q16(const BfmForestSpeechActor *actor)
{
    return actor && actor->standing_step == COASTING ? -(4 * 65536) : 0;
}
