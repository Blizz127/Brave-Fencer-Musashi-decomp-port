/* SPDX-License-Identifier: MIT */
#include "steam_knight_event.h"
#include "forest_walk.h"

enum {
    REVEAL_HALL = 234, ENTER_STRIDE = 11, APPROACH_STRIDE = 12,
    FORM_RING = 16, TRAP_READY = 33, SPIN_RECOVERY = 24
};
/* Offsets within the entrance's own dialogue block. The disc supplies all
 * text, voice cues, icon commands and page holds, including the tutorial. */
static const uint16_t dialogue_offsets[] = {
    0, 0x44, 0x98, 0xc8, 0x1a4, 0x1f4, 0x25c, 0x294, 0x2b4, 0x2f4, 0x318, 0x3f0,
    0x430, 0x498, 0x4f4, 0x558, 0x588, 0x5c4, 0x5e0, 0x610
};
enum {
    ARRIVAL_SPEECH, CAPTOR_INTRO, PRINCESS_CALL, CAPTOR_BARGAIN, MUSASHI_REFUSAL,
    CAPTOR_FLAMES, MUSASHI_TRAPPED, MUSASHI_QUESTION, PRINCESS_ADVICE,
    MUSASHI_RECALL, ROTATION_TUTORIAL, MUSASHI_REPLY, FILLET_PRAISE, CAPTOR_THREAT,
    CAPTOR_FAREWELL, MUSASHI_WAIT, MUSASHI_HEARS_MACHINE, MUSASHI_SEES_MACHINE, MUSASHI_CHALLENGE
};
static void stage(BfmSteamKnightEvent *e, BfmSteamStage next)
{
    e->stage = next;
    e->stage_age = 0;
}
static int speak(BfmSteamKnightEvent *e, unsigned step, BfmSteamShot shot)
{
    size_t offset = dialogue_offsets[step];
    e->message = bfm_field_message_borrow(e->programs + offset, dialogue_offsets[step + 1] - offset);
    if (!e->message) return 0;
    e->step = step;
    e->message_age = 0;
    e->shot = shot;
    return 1;
}
int bfm_steam_event_begin(BfmSteamKnightEvent *e, const uint8_t *scene, size_t bytes,
                          const RoomTransferDescriptor *entry, BfmGameplayRun *run, ForestVoice *voice)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (!e || !scene || !entry || entry->room != 0x300e || entry->parameter || entry->world ||
        !run || !run->has_player || !bfm_gameplay_story_flag(run, 128) || !(run->player.record[0x48] & 1) ||
        !voice || entry->scene_data < origin)
        return 0;
    size_t offset = entry->scene_data - origin;
    if (offset > bytes || bytes - offset < dialogue_offsets[MUSASHI_CHALLENGE + 1]) return 0;
    for (unsigned i = ARRIVAL_SPEECH; i <= MUSASHI_CHALLENGE; ++i)
        if (scene[offset + dialogue_offsets[i]] != 5) return 0;
    *e = (BfmSteamKnightEvent){ .programs = scene + offset, .run = run, .voice = voice,
        .position = { (int16_t)entry->x, (int16_t)entry->y, (int16_t)entry->z },
        .heading = (int16_t)entry->facing };
    return 1;
}
void bfm_steam_event_clear(BfmSteamKnightEvent *e)
{
    if (!e) return;
    bfm_forest_message_free(e->message);
    e->message = NULL;
}

static int finish_page(BfmSteamKnightEvent *e)
{
    switch (e->step) {
    case ARRIVAL_SPEECH: return speak(e, CAPTOR_INTRO, BFM_STEAM_CAPTOR_CAMERA);
    case CAPTOR_INTRO: return speak(e, PRINCESS_CALL, BFM_STEAM_CAPTOR_CAMERA);
    case PRINCESS_CALL: return speak(e, CAPTOR_BARGAIN, BFM_STEAM_CAPTOR_CAMERA);
    case CAPTOR_BARGAIN: return speak(e, MUSASHI_REFUSAL, BFM_STEAM_CAPTOR_CAMERA);
    case MUSASHI_REFUSAL:
        stage(e, BFM_STEAM_APPROACH);
        e->wait = 14;
        return 1;
    case CAPTOR_FLAMES: return speak(e, MUSASHI_TRAPPED, BFM_STEAM_FIRE_CAMERA);
    case MUSASHI_TRAPPED: return speak(e, MUSASHI_QUESTION, BFM_STEAM_FIRE_CAMERA);
    case MUSASHI_QUESTION: return speak(e, PRINCESS_ADVICE, BFM_STEAM_PRINCESS_CAMERA);
    case PRINCESS_ADVICE: return speak(e, MUSASHI_RECALL, BFM_STEAM_FIRE_CAMERA);
    case MUSASHI_RECALL: return speak(e, ROTATION_TUTORIAL, BFM_STEAM_TUTORIAL_CAMERA);
    case ROTATION_TUTORIAL:
        stage(e, BFM_STEAM_READY);
        return 1;
    case MUSASHI_REPLY:
        stage(e, BFM_STEAM_HOSTAGE_PRAISE);
        return speak(e, FILLET_PRAISE, BFM_STEAM_REPLY_CAMERA);
    case FILLET_PRAISE:
        stage(e, BFM_STEAM_SILENCE);
        return 1;
    case CAPTOR_THREAT:
        stage(e, BFM_STEAM_FAREWELL);
        return speak(e, CAPTOR_FAREWELL, BFM_STEAM_CARPET_CAMERA);
    case CAPTOR_FAREWELL:
        stage(e, BFM_STEAM_CHASE);
        return speak(e, MUSASHI_WAIT, BFM_STEAM_CARPET_CAMERA);
    case MUSASHI_WAIT:
        stage(e, BFM_STEAM_RUMBLE);
        return 1;
    case MUSASHI_HEARS_MACHINE:
        stage(e, BFM_STEAM_LOOK_UP);
        return 1;
    case MUSASHI_SEES_MACHINE:
        stage(e, BFM_STEAM_DESCENT);
        return 1;
    case MUSASHI_CHALLENGE:
        stage(e, BFM_STEAM_DUEL);
        e->shot = BFM_STEAM_DUEL_CAMERA;
        return 1;
    default: return 0;
    }
}
static int forward(BfmSteamKnightEvent *e, int target, int stride)
{
    int z = e->position[2] + stride;
    e->position[2] = (int16_t)(z < target ? z : target);
    return z >= target;
}
/* The feet keep moving during speech. The challenge starts one normal
 * step before Musashi reaches his destination. */
static void postfire_motion(BfmSteamKnightEvent *e)
{
    if (e->stage == BFM_STEAM_HOSTAGE_PRAISE && e->stage_age >= 6)
        e->shot = BFM_STEAM_PRINCESS_CAMERA;
    if (e->stage == BFM_STEAM_CHASE && e->stage_age >= 3)
        forward(e, -3840, ENTER_STRIDE);
    if (e->stage == BFM_STEAM_LOOK_UP && e->stage_age >= 40)
        e->shot = BFM_STEAM_ROOF_CAMERA;
    if (e->stage == BFM_STEAM_BREACH || e->stage == BFM_STEAM_DESCENT ||
        e->stage == BFM_STEAM_CHALLENGE)
        ++e->landing_age;
    if (e->stage == BFM_STEAM_CHALLENGE)
        forward(e, -3456, ENTER_STRIDE);
}
static void charge(BfmSteamKnightEvent *e, uint16_t pressed, uint16_t held)
{
    uint8_t *gauge = &e->run->player.record[0x47];
    if (!(held & BFM_FOREST_R1)) {
        *gauge = 0;
        e->grip = 0;
    } else if (e->grip && (pressed & BFM_FOREST_TRIANGLE) && *gauge == 128) {
        *gauge = 0;
        e->grip = 0;
        stage(e, BFM_STEAM_ROTATING);
    } else if (e->grip) {
        *gauge = *gauge >= 125 ? 128 : (uint8_t)(*gauge + 3);
    } else {
        e->grip = 1;
        *gauge = 0;
    }
}
int bfm_steam_event_tick(BfmSteamKnightEvent *e, uint16_t pressed, uint16_t held, unsigned live_flames)
{
    if (!e || live_flames > 8) return -1;
    if (e->stage == BFM_STEAM_DUEL) return 0;
    ++e->age;
    ++e->stage_age;
    postfire_motion(e);
    bfm_forest_voice_step(e->voice);
    if (e->message) {
        ++e->message_age;
        const BfmForestMessageFrame *view = bfm_forest_voice_page(e->voice, e->message, &e->voice_generation,
                                   !!(pressed & BFM_FOREST_CROSS), !!(pressed & BFM_FOREST_TRIANGLE));
        if (!view) return -1;
        if (e->stage == BFM_STEAM_REPLY && view->text_visible) e->shot = BFM_STEAM_REPLY_CAMERA;
        if (view->events & BFM_FOREST_MESSAGE_CLOSED) {
            bfm_steam_event_clear(e);
            if (!finish_page(e)) return -1;
        }
        return 1;
    }
    if (e->wait) {
        --e->wait;
        return 1;
    }
    switch (e->stage) {
    case BFM_STEAM_ARRIVAL:
        if (e->age >= REVEAL_HALL) {
            stage(e, BFM_STEAM_ENTER_HALL);
            e->wait = 1;
        }
        break;
    case BFM_STEAM_ENTER_HALL:
        if (forward(e, -4352, ENTER_STRIDE)) {
            stage(e, BFM_STEAM_EXCHANGE);
            if (!speak(e, ARRIVAL_SPEECH, BFM_STEAM_ENTRY_CAMERA)) return -1;
        }
        break;
    case BFM_STEAM_APPROACH: {
        e->shot = BFM_STEAM_HALL_CAMERA;
        int arrived = forward(e, -3968, APPROACH_STRIDE);
        e->position[1] = (int16_t)(-2178 + (e->position[2] + 4352) / 3);
        if (arrived) stage(e, BFM_STEAM_ENCLOSURE);
        break;
    }
    case BFM_STEAM_ENCLOSURE:
        if (e->stage_age >= TRAP_READY) {
            stage(e, BFM_STEAM_INSTRUCTION);
            if (!speak(e, CAPTOR_FLAMES, BFM_STEAM_CAPTOR_LOW_CAMERA)) return -1;
        }
        break;
    case BFM_STEAM_READY:
        charge(e, pressed, held);
        break;
    case BFM_STEAM_ROTATING:
        if (e->stage_age >= SPIN_RECOVERY) {
            if (live_flames) stage(e, BFM_STEAM_READY);
            else {
                stage(e, BFM_STEAM_REPLY);
                if (!speak(e, MUSASHI_REPLY, BFM_STEAM_TUTORIAL_CAMERA)) return -1;
            }
        }
        break;
    case BFM_STEAM_SILENCE:
        if (e->stage_age == 30 && !speak(e, CAPTOR_THREAT, BFM_STEAM_PRINCESS_CAMERA)) return -1;
        break;
    case BFM_STEAM_RUMBLE:
        if (e->stage_age == 30 && !speak(e, MUSASHI_HEARS_MACHINE, BFM_STEAM_CARPET_CAMERA)) return -1;
        break;
    case BFM_STEAM_LOOK_UP:
        if (e->stage_age >= 65) {
            stage(e, BFM_STEAM_BREACH);
            e->shot = BFM_STEAM_ROOF_CAMERA;
            e->landing_age = 0;
        }
        break;
    case BFM_STEAM_BREACH:
        if (e->stage_age == 24 && !speak(e, MUSASHI_SEES_MACHINE, BFM_STEAM_ROOF_CAMERA)) return -1;
        break;
    case BFM_STEAM_DESCENT:
        e->shot = BFM_STEAM_MACHINE_CAMERA;
        if (e->stage_age >= 150) stage(e, BFM_STEAM_CHALLENGE);
        break;
    case BFM_STEAM_CHALLENGE:
        if (e->position[2] >= -3467 && !speak(e, MUSASHI_CHALLENGE, BFM_STEAM_MACHINE_CAMERA)) return -1;
        break;
    default: return -1;
    }
    return 1;
}
void bfm_steam_event_captor_pose(const BfmSteamKnightEvent *e, unsigned *clip, unsigned *age)
{
    *clip = 0;
    *age = e->age;
    if (!e->message) return;
    /* The fist rises briefly during the introduction. The later bargain
     * has a longer, separate speaking gesture. Both return to standing. */
    if (e->step == CAPTOR_INTRO && e->message_age >= 13 && e->message_age < 34) {
        *clip = 6;
        *age = e->message_age - 13;
    } else if (e->step == CAPTOR_BARGAIN && e->message_age >= 2 && e->message_age < 52) {
        *clip = 4;
        *age = e->message_age - 2;
    }
}
int bfm_steam_event_show_captors(const BfmSteamKnightEvent *e)
{
    return e->stage < BFM_STEAM_CHASE || (e->stage == BFM_STEAM_CHASE && e->stage_age < 3);
}
int bfm_steam_event_title(const BfmSteamKnightEvent *e)
{
    return e->stage == BFM_STEAM_DESCENT;
}
int bfm_steam_event_ring_started(const BfmSteamKnightEvent *e)
{
    return e->stage > BFM_STEAM_ENCLOSURE ||
           (e->stage == BFM_STEAM_ENCLOSURE && e->stage_age >= FORM_RING);
}
int bfm_steam_event_hud(const BfmSteamKnightEvent *e)
{
    if (e->stage == BFM_STEAM_DUEL) return 1;
    if (e->shot == BFM_STEAM_REPLY_CAMERA || e->stage > BFM_STEAM_REPLY) return 0;
    return e->stage >= BFM_STEAM_READY ||
           (e->stage == BFM_STEAM_INSTRUCTION && e->step == ROTATION_TUTORIAL);
}
unsigned bfm_steam_event_shade(const BfmSteamKnightEvent *e)
{
    if (e->age < 14) return 0;
    if (e->age < 30) return (e->age - 14) * 2;
    if (e->age < REVEAL_HALL) return 31;
    unsigned since = e->age - REVEAL_HALL;
    if (since <= 2) return 31;
    return since < 18 ? 31 - 2 * (since - 2) : 0;
}
