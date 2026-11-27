/* SPDX-License-Identifier: MIT */
#include "woods_caster_melee.h"
#include "woods_charged_hit.h"
#include "bedroom_effects.h"
#include "bedroom_figure.h"
#include "forest_field.h"
#include "forest_swing.h"
#include "steam_knight_spin_contact.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t pose;
    int8_t tip[3];
    uint8_t window, marker, show_lumina;
} StrikeKey;

typedef struct {
    StrikeKey keys[BFM_FOREST_SWING_KEYS];
    unsigned count;
} StrikeClip;

typedef struct {
    BfmWeaponPower power;
    unsigned event;
} Strike;

enum { FUSION_FIRST, FUSION_SECOND, FUSION_THIRD, LUMINA, CHARGED, CLIPS };

struct BfmWoodsCasterMelee {
    StrikeClip clips[CLIPS];
    Strike moves[6]; /* Alternating starting contacts, two follow-ups and both Lumina contacts. */
    BfmWoodsChargedWeapon weapon;
    const uint8_t *scene;
    size_t scene_bytes;
    unsigned action, variant, key, active_updates;
    int alternate, connected;
};

static const uint8_t *mounted(const uint8_t *payload, size_t bytes, uint32_t base,
                              uint32_t address, size_t count)
{
    if (!payload || address < base || (address & 3)) return NULL;
    size_t offset = address - base;
    return offset <= bytes && count <= bytes - offset ? payload + offset : NULL;
}

/* Grounded attack clips consist of individual body keys interspersed with
 * weapon/window changes. Both stop and final-key terminators finish the move. */
static int load_clip(StrikeClip *clip, const uint8_t *main, size_t bytes,
                     uint32_t address, int lumina)
{
    const uint32_t base = UINT32_C(0x800cedf8);
    unsigned window = 0, marker = 0, show_lumina = 0;
    for (unsigned i = 0; i < 2 * BFM_FOREST_SWING_KEYS; ++i) {
        const uint8_t *word = mounted(main, bytes, base, address + i * 4, 4);
        if (!word) return 0;
        uint32_t command = u32le(word);
        switch (command) {
        case 0:
        case 2:
            return clip->count > 2 && !window && !marker;
        case 4: {
            const uint8_t *model = mounted(main, bytes, base, address + (++i) * 4, 4);
            if (!model || !mounted(main, bytes, base, u32le(model), 4)) return 0;
            break;
        }
        case 7: marker = 1; break;
        case 16: /* Keep the equipped Fusion in hand. */
            break;
        case 18: show_lumina = 1; break;
        case 19: show_lumina = 0; break;
        case 20: window ^= 1; break;
        default: {
            const uint8_t *packet = mounted(main, bytes, base, command, 256);
            if (!packet || clip->count == BFM_FOREST_SWING_KEYS) return 0;
            StrikeKey *key = &clip->keys[clip->count++];
            key->pose = command + 16;
            key->window = (uint8_t)window;
            key->marker = (uint8_t)marker;
            marker = 0;
            key->show_lumina = (uint8_t)show_lumina;
            memcpy(key->tip, packet + (lumina ? 8 : 0), sizeof key->tip);
            break;
        }
        }
    }
    return 0;
}

static int load_move(Strike *strike, const uint8_t *scene, size_t bytes,
                     unsigned move, int lumina)
{
    const uint32_t base = UINT32_C(0x80128158);
    const uint8_t *row = mounted(scene, bytes, base, UINT32_C(0x801896d0) + move * 20, 20);
    if (!row || u32le(row + 4) != (unsigned)lumina || u16le(row + 18) != (lumina ? 2 : 1))
        return 0;
    unsigned event = u32le(row);
    if (!event || event > UINT16_MAX) return 0;
    const uint8_t *power = mounted(scene, bytes, base, UINT32_C(0x80189874) + event * 12, 12);
    if (!power) return 0;
    unsigned flags = u16le(power);
    if ((flags & 0x7000) != 0x4000) return 0;
    strike->event = event;
    strike->power = (BfmWeaponPower){flags & 0xfff, u16le(power + 2),
                                     u16le(power + 4), u16le(power + 6), !!(flags & 0x8000)};
    return 1;
}

BfmWoodsCasterMelee *bfm_woods_caster_melee_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes)
{
    static const uint32_t script[CHARGED] = {
        UINT32_C(0x800d4c94), UINT32_C(0x800d4ce4), UINT32_C(0x800d4d38), UINT32_C(0x800d4f14)
    };
    BfmWoodsCasterMelee *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    for (unsigned i = 0; i < CHARGED; ++i)
        if (!load_clip(&s->clips[i], main, main_bytes, script[i], i == LUMINA)) goto fail;
    for (unsigned i = 0; i < 6; ++i)
        if (!load_move(&s->moves[i], scene, scene_bytes, i < 4 ? i : i + 6, i >= 4)) goto fail;
    /* The room's body bank names the equipped sword cut at slot 23. */
    const uint32_t resident = UINT32_C(0x800cedf8);
    const uint8_t *bank = mounted(scene, scene_bytes, UINT32_C(0x80128158),
                                  UINT32_C(0x8018ea38) + 0x4c, 4);
    if (!bank) goto fail;
    const uint8_t *slots = mounted(main, main_bytes, resident, u32le(bank), 24 * 4);
    if (!slots || !load_clip(&s->clips[CHARGED], main, main_bytes, u32le(slots + 23 * 4), 1) ||
        !bfm_woods_charged_hit_load(scene, scene_bytes, &s->weapon.hit) ||
        !bfm_weapon_ranks_load(disc, s->weapon.ranks)) goto fail;
    s->scene = scene;
    s->scene_bytes = scene_bytes;
    return s;
fail:
    free(s);
    return NULL;
}

const BfmWoodsChargedWeapon *bfm_woods_caster_melee_weapon(const BfmWoodsCasterMelee *s)
{
    return s ? &s->weapon : NULL;
}

void bfm_woods_caster_melee_free(BfmWoodsCasterMelee *s)
{
    free(s);
}

int bfm_woods_caster_melee_cut_finishing(const BfmWoodsCasterMelee *s)
{
    return s && s->action == 19 && s->key + 1 >= s->clips[CHARGED].count;
}

void bfm_woods_caster_melee_connected(BfmWoodsCasterMelee *s)
{
    if (s && s->action == 5) s->connected = 1;
}

static void start(BfmWoodsCasterMelee *s, unsigned action, unsigned variant,
                  BfmWoodsCasterMeleeFrame *frame)
{
    if (variant < 2) {
        s->alternate ^= 1;
        variant = (action == 6 ? 4u : 0u) + !s->alternate;
    }
    s->action = frame->action = action;
    s->variant = variant;
    s->key = s->active_updates = 0;
    s->connected = 0;
    frame->sound = action == 6 ? 1109 : variant == 2 ? 1094 : 1024;
}

int bfm_woods_caster_melee_step(BfmWoodsCasterMelee *s, BfmGameplayRun *run,
    const BfmBedroomRoot *root, const BfmWoodsCasterMeleeInput *input,
    BfmWoodsCasterMeleeFrame *frame)
{
    if (!s || !run || !root || !input || !frame) return 0;
    *frame = (BfmWoodsCasterMeleeFrame){0};
    if (!input->permitted || ((input->actor_flags & 0x400) && !input->charged_counter && s->action != 19)) {
        if (s->action == 19) frame->face_slot = 17;
        s->action = 0;
        return 1;
    }
    if (!s->action) {
        if (input->charged_counter) {
            if (input->cut_support_changed) return 1;
            s->action = frame->action = 19;
            s->key = 0;
            frame->face_slot = 21;
            frame->charge_started = 1;
            frame->selected_pose = s->clips[CHARGED].keys[0].pose;
        } else if (input->square_edge && !(input->actor_flags & 4)) start(s, 5, 0, frame);
        else if (input->triangle_edge && bfm_gameplay_story_flag(run, 128)) start(s, 6, 0, frame);
        return 1;
    }
    unsigned which = s->action == 19 ? CHARGED : s->variant >= 4 ? LUMINA :
                     s->variant < 2 ? FUSION_FIRST : s->variant - 1;
    const StrikeClip *clip = &s->clips[which];
    if (++s->key >= clip->count) {
        if (s->action == 19) frame->face_slot = 17;
        s->action = 0;
        return 1;
    }
    const StrikeKey *key = &clip->keys[s->key];
    if (s->action == 5 && s->active_updates >= 4 && input->square_edge) {
        if (s->connected && s->variant < 3) {
            start(s, 5, s->variant < 2 ? 2 : 3, frame);
            return 1;
        }
        if (!s->connected && (s->variant < 2 || !key->window)) {
            start(s, 5, 0, frame);
            return 1;
        }
    }
    if (s->action == 5 && !key->window && s->active_updates && (input->moving || input->jump_edge)) {
        s->action = 0;
        return 1;
    }
    frame->action = s->action;
    if (s->action == 19) {
        if (key->window && s->key + 1 == clip->count) return 0;
        frame->face_slot = 21;
        frame->charge_marker = key->marker;
        if (key->marker) frame->sound = BFM_WOODS_CHARGED_SLASH_SOUND;
        frame->selected_pose = key->pose;
        if (input->cut_support_changed) {
            /* A synchronized effect is issued before the ordinary support
             * transition, but a new fall/slide does not resolve this strike. */
            s->action = frame->action = 0;
            frame->face_slot = 0;
            return 1;
        }
        if (!bfm_woods_charged_hit_step(&s->weapon.hit, s->weapon.ranks, run, root,
                input->actor_flags, key->window, key->tip,
                key->window ? clip->keys[s->key + 1].tip : NULL, frame)) return 0;
    } else {
        const Strike *strike = &s->moves[s->variant];
        BfmSteamSpinRoll roll;
        if (!bfm_weapon_roll(s->weapon.ranks, &strike->power, run, input->actor_flags, &roll)) return 0;
        frame->event = strike->event;
        frame->strength = roll.strength;
        frame->critical = (unsigned)roll.critical;
        if (key->window) {
            if (s->key + 1 >= clip->count) return 0;
            ++s->active_updates;
            BfmForestTransform transform;
            if (!bfm_bedroom_figure_actor_root(root, 0, &transform)) return 0;
            int8_t next[3];
            memcpy(next, clip->keys[s->key + 1].tip, sizeof next);
            /* The ordinary Lumina sweep shares its following sample's vertical
             * component between X and Y in the secondary chord. */
            if (s->action == 6) next[0] = next[1];
            frame->rays = (unsigned)bfm_forest_blade_rays(&transform, key->tip, next,
                                                         frame->from, frame->to);
        }
    }
    if (!bfm_woods_weapon_credit_read(s->scene, s->scene_bytes, frame->event, run,
                                    &s->weapon.credit)) return 0;
    if (s->key >= 2) {
        const StrikeKey *draw = &clip->keys[s->key - 2];
        frame->pose = draw->pose;
        frame->show_fusion = !(input->actor_flags & 4) && !bfm_gameplay_story_flag(run, 303);
        frame->show_lumina = draw->show_lumina;
    }
    return 1;
}
