/* SPDX-License-Identifier: MIT */
#include "steam_knight_finale.h"
#include <string.h>

int bfm_steam_finale_begin(BfmSteamKnightFinale *f, const BfmSteamFinalImpact *hit,
                           const BfmForestLiftData *lift)
{
    if (!f || f->phase != BFM_STEAM_FINAL_INACTIVE || !hit || hit->round != 2 ||
        hit->phase != 30 || hit->chassis_stage != 13 || hit->chassis_hp ||
        hit->launch_action != 14 || !hit->grounded_launch || !hit->struck_last_structure || !lift)
        return 0;
    for (unsigned i = BFM_LIFT_RAISE; i <= BFM_LIFT_THROW; ++i)
        if (!lift->motion[i].count || lift->motion[i].count > BFM_LIFT_KEYS) return 0;
    const BfmLiftMotion *toss = &lift->motion[BFM_LIFT_THROW];
    if (!toss->event || toss->event >= toss->count)
        return 0;
    *f = (BfmSteamKnightFinale){.phase = BFM_STEAM_FINAL_THROW, .lift = lift};
    memcpy(f->hero, hit->hero, sizeof f->hero);
    return 1;
}

static void enter(BfmSteamKnightFinale *f, BfmSteamFinalPhase phase)
{
    f->phase = phase;
    f->age = 0;
    f->page_closed_at = 0;
}

static void hero_motion(BfmSteamKnightFinale *f, BfmSteamFinalHero motion)
{
    f->hero_motion = motion;
    f->motion_age = 0;
}

static int speak(BfmSteamKnightFinale *f, BfmSteamKnightEvent *e, BfmSteamFinalSpeech which)
{
    e->message = bfm_steam_event_final_speech(e, which);
    f->page_open = e->message != NULL;
    return f->page_open;
}

static int mace_tick(BfmSteamKnightFinale *f, BfmSteamKnightEvent *e)
{
    if (f->age == 83) {
        /* The staged shot seats Musashi on the ledge. The mace he raises
         * is a different object from the chassis at the bottom. */
        f->hero[0] = -1376;
        f->hero[1] = -2050;
        f->hero[2] = 2944;
        f->mace = BFM_STEAM_FINAL_MACE_HELD;
        hero_motion(f, BFM_STEAM_FINAL_HERO_RAISE);
    }
    if (f->hero_motion == BFM_STEAM_FINAL_HERO_RAISE &&
        f->motion_age == f->lift->motion[BFM_LIFT_RAISE].count)
        hero_motion(f, BFM_STEAM_FINAL_HERO_HOLD);
    if (f->age == 102) f->display_hp = 30;
    if (f->age == 126) hero_motion(f, BFM_STEAM_FINAL_HERO_WALK);
    if (f->age >= 126 && f->age < 171) f->hero[0] -= 4;
    if (f->age == 171) {
        f->hero[0] = -1568;
        hero_motion(f, BFM_STEAM_FINAL_HERO_HOLD);
        if (!speak(f, e, BFM_STEAM_FINAL_THROW_TAUNT)) return -1;
    }
    if (f->age == 306) hero_motion(f, BFM_STEAM_FINAL_HERO_TOSS);
    if (f->hero_motion == BFM_STEAM_FINAL_HERO_TOSS) {
        const BfmLiftMotion *toss = &f->lift->motion[BFM_LIFT_THROW];
        if (f->motion_age == toss->event) f->mace = BFM_STEAM_FINAL_MACE_AIRBORNE;
        if (f->motion_age == toss->count) hero_motion(f, BFM_STEAM_FINAL_HERO_IDLE);
    }
    if (f->age == 343) {
        if (f->mace != BFM_STEAM_FINAL_MACE_AIRBORNE || f->display_hp != 30) return -1;
        f->mace = BFM_STEAM_FINAL_MACE_HIT;
        f->display_hp = 0;
    }
    if (f->age == 391) enter(f, BFM_STEAM_FINAL_BLAST);
    return BFM_STEAM_FINALE_PLAY;
}

static int update(BfmSteamKnightFinale *f, BfmSteamKnightEvent *e, BfmSteamFinalBodyEvent body)
{
    if (!!e->message != !!f->page_open) return -1;
    bfm_forest_voice_step(e->voice);
    if (f->page_open) {
        const BfmForestMessageFrame *frame =
            bfm_forest_voice_page(e->voice, e->message, &e->voice_generation, 0, 0);
        if (!frame) return -1;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
            bfm_steam_event_clear(e);
            f->page_open = 0;
            f->page_closed_at = f->age + 1;
        }
    }
    if (body == BFM_STEAM_FINAL_REPLAY) {
        if (f->phase != BFM_STEAM_FINAL_THROW || f->age < 44) return -1;
        enter(f, BFM_STEAM_FINAL_MACE);
        return BFM_STEAM_FINALE_PLAY;
    }
    if (body == BFM_STEAM_FINAL_PIT_LANDING) {
        if (f->phase != BFM_STEAM_FINAL_MACE || f->age < 54 || f->landed) return -1;
        f->landed = 1;
    }
    if (body == BFM_STEAM_FINAL_LEDGE) {
        if (f->phase != BFM_STEAM_FINAL_MACE || !f->landed || f->age != 82 || f->ledge) return -1;
        f->ledge = 1;
    }
    /* A missing landing holds the ledge sequence. A slow taunt can hold the
     * toss. Neither an elapsed deadline nor a pad edge completes a page. */
    if (f->phase == BFM_STEAM_FINAL_MACE &&
        ((f->age == 54 && !f->landed) || (f->age == 82 && !f->ledge) ||
         (f->age == 305 && !f->page_closed_at)))
        return BFM_STEAM_FINALE_PLAY;
    ++f->age;
    ++f->motion_age;
    switch (f->phase) {
    case BFM_STEAM_FINAL_THROW:
        break;
    case BFM_STEAM_FINAL_MACE:
        return mace_tick(f, e);
    case BFM_STEAM_FINAL_BLAST:
        if (f->age == 103) enter(f, BFM_STEAM_FINAL_WHITE);
        break;
    case BFM_STEAM_FINAL_WHITE:
        if (f->age == 17) {
            enter(f, BFM_STEAM_FINAL_AFTERMATH);
            return BFM_STEAM_FINALE_ENTERED_AFTERMATH;
        }
        break;
    case BFM_STEAM_FINAL_AFTERMATH:
        if (f->age == 58 && !speak(f, e, BFM_STEAM_FINAL_RECOLLECTION)) return -1;
        if (f->exit_started) {
            f->retired = 1;
            return BFM_STEAM_FINALE_RETIRED;
        }
        if (f->age >= 371 && f->page_closed_at && f->age - f->page_closed_at >= 25 &&
            bfm_forest_voice_status(e->voice) == BFM_FOREST_VOICE_IDLE) {
            f->exit_started = 1;
            return BFM_STEAM_FINALE_EXIT_START;
        }
        break;
    default:
        return -1;
    }
    return BFM_STEAM_FINALE_PLAY;
}

int bfm_steam_finale_tick(BfmSteamKnightFinale *f, BfmSteamKnightEvent *e, BfmSteamFinalBodyEvent body)
{
    if (!f || !e || !e->voice || e->stage != BFM_STEAM_DUEL || f->failed ||
        f->phase == BFM_STEAM_FINAL_INACTIVE || (unsigned)body > BFM_STEAM_FINAL_LEDGE)
        return -1;
    if (f->retired) return BFM_STEAM_FINALE_PLAY;
    int signal = update(f, e, body);
    if (signal < 0) f->failed = 1;
    return signal;
}

int bfm_steam_finale_boss_hud(const BfmSteamKnightFinale *f)
{
    return f && f->phase >= BFM_STEAM_FINAL_THROW && f->phase <= BFM_STEAM_FINAL_BLAST;
}

int bfm_steam_finale_crater_shot(const BfmSteamKnightFinale *f)
{
    return f && f->phase == BFM_STEAM_FINAL_AFTERMATH && f->age >= 54;
}

uint32_t bfm_steam_finale_body(const BfmSteamKnightFinale *f)
{
    static const BfmLiftClip clips[] = {
        [BFM_STEAM_FINAL_HERO_RAISE] = BFM_LIFT_RAISE, [BFM_STEAM_FINAL_HERO_HOLD] = BFM_LIFT_IDLE,
        [BFM_STEAM_FINAL_HERO_WALK] = BFM_LIFT_WALK, [BFM_STEAM_FINAL_HERO_TOSS] = BFM_LIFT_THROW
    };
    if (!f || !f->lift || f->hero_motion < BFM_STEAM_FINAL_HERO_RAISE ||
        f->hero_motion > BFM_STEAM_FINAL_HERO_TOSS) return 0;
    const BfmLiftMotion *motion = &f->lift->motion[clips[f->hero_motion]];
    return motion->frames[f->motion_age % motion->count];
}

void bfm_steam_finale_flash(const BfmSteamKnightFinale *f, uint32_t pixels[240][320])
{
    if (!f || !pixels) return;
    int gain;
    if (f->phase == BFM_STEAM_FINAL_BLAST) {
        if (f->age < 82) return;
        gain = 1 + 2 * ((int)f->age - 82);
        if (gain > 31) gain = 31;
    } else if (f->phase == BFM_STEAM_FINAL_WHITE || f->phase == BFM_STEAM_FINAL_AFTERMATH) {
        unsigned elapsed = f->age + (f->phase == BFM_STEAM_FINAL_AFTERMATH ? 17 : 0);
        if (elapsed >= 83) return;
        gain = 31 - (int)(3 * elapsed / 8);
    } else return;
    for (unsigned y = 10; y < 230; ++y) {
        for (unsigned x = 0; x < 320; ++x) {
            uint32_t source = pixels[y][x], out = source & UINT32_C(0xff000000);
            for (unsigned shift = 0; shift <= 16; shift += 8) {
                int level = (int)((source >> (shift + 3)) & 31u) + gain;
                if (level > 31) level = 31;
                out |= (uint32_t)(level * 255 / 31) << shift;
            }
            pixels[y][x] = out;
        }
    }
}
