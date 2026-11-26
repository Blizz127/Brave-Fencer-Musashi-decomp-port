/* SPDX-License-Identifier: MIT */
#include "steam_knight.h"
#include "steam_knight_event.h"
#include "steam_knight_cast.h"
#include "steam_knight_rotation.h"
#include "steam_knight_effects.h"
#include "steam_knight_entry_light.h"
#include "steam_knight_machine.h"
#include "steam_knight_armor.h"
#include "steam_knight_core_damage.h"
#include "steam_knight_core_origin.h"
#include "steam_knight_hero_motion.h"
#include "steam_knight_boss_hud.h"
#include "forest_hud.h"
#include "forest_damage_digits.h"
#include "forest_message_art.h"
#include <stdlib.h>
#include <string.h>

enum { SELECT_HALL = 30, PLACE_HERO = 188 };
struct BfmSteamKnight {
    BfmSteamKnightField *field;
    BfmForestMessageArt *art;
    ForestVoice *voice;
    BfmForestHud *hud;
    BfmSteamKnightHostages *hostages;
    BfmSteamKnightMachine *machine;
    const MusashiDiscMedia *disc;
    BfmSteamKnightHeroMotion *hero;
    BfmSteamKnightArmor armor;
    BfmForestFusionPower power;
    BfmSteamKnightArmorHit hit;
    struct {
        BfmForestDamageDigits digits;
        unsigned delay;
    } numbers[BFM_STEAM_ARMOR_PLATES];
    const uint8_t *damage_palette;
    int swing_sound, hit_sound;
    BfmSteamKnightBossHud boss_hud;
    BfmSteamKnightCoreBoxes core_boxes;
    unsigned core_hp, core_max;
    unsigned round, weak_stage, weak_age, knee_age, core_struck, recovery_left;
    unsigned body_stage, pending_kneel_joint;
    uint32_t battle_flags, pending_kneel_flags;
    BfmLuminaRotation rotation;
    BfmSteamWeaponEffects weapons;
    BfmSteamKnightFlames flames;
    int ring_started, hud_shown;
    BfmSteamKnightEvent event;
};

BfmSteamKnight *bfm_steam_knight_open(const MusashiDiscMedia *disc,
    const RoomTransferDescriptor *request, BfmGameplayRun *run)
{
    if (!disc || !request || request->room != 0x300e || request->parameter ||
        request->world || !run || !run->has_player || !bfm_gameplay_story_flag(run, 128) ||
        !(run->player.record[0x48] & 1))
        return NULL;
    BfmSteamKnight *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->disc = disc;
    s->field = bfm_steam_knight_field_open(disc);
    if (!s->field) goto fail;
    size_t size;
    const uint8_t *scene = bfm_steam_knight_field_scene(s->field, &size);
    const RoomTransferDescriptor *entry = bfm_steam_knight_field_entrance(s->field);
    const uint16_t (*tiles)[1024] = bfm_forest_terrain_tiles(bfm_steam_knight_field_terrain(s->field));
    s->art = bfm_castle_message_art_load(disc, scene, size, tiles, 0x68e58);
    s->hostages = bfm_steam_knight_hostages_load(disc, size);
    const size_t damage_palette = 0x80193a44u - 0x80128158u;
    if (damage_palette + 2 > size) goto fail;
    s->damage_palette = scene + damage_palette;
    s->machine = bfm_steam_knight_machine_load(scene, size);
    s->core_max = s->core_hp = 192;
    s->body_stage = s->weak_stage = 1;
    s->hud = bfm_steam_knight_hud_load(disc, scene, size, bfm_steam_knight_field_terrain(s->field));
    s->voice = bfm_forest_voice_load(disc);
    if (!s->art || !s->hostages || !s->machine || !s->hud ||
        !bfm_steam_knight_armor_load(&s->armor, scene, size) ||
        !bfm_steam_knight_core_boxes_load(&s->core_boxes, scene, size) ||
        !bfm_steam_knight_armor_load_power(disc, scene, size, &s->power) ||
        !bfm_steam_knight_boss_hud_load(&s->boss_hud, disc, scene, size) || !bfm_lumina_rotation_load(disc, &s->rotation) ||
        !bfm_steam_weapons_load(&s->weapons, scene, size) ||
        !bfm_steam_knight_flames_load(&s->flames, scene, size) ||
        !bfm_steam_event_begin(&s->event, scene, size, entry, run, s->voice)) goto fail;
    return s;
fail:
    bfm_steam_knight_free(s);
    return NULL;
}

void bfm_steam_knight_free(BfmSteamKnight *s)
{
    if (!s) return;
    bfm_steam_event_clear(&s->event);
    bfm_steam_hero_free(s->hero);
    bfm_steam_knight_machine_free(s->machine);
    bfm_steam_knight_hostages_free(s->hostages);
    bfm_forest_hud_free(s->hud);
    bfm_forest_voice_free(s->voice);
    bfm_forest_message_art_free(s->art);
    bfm_steam_knight_field_free(s->field);
    free(s);
}

static void hall_camera(const BfmSteamKnight *s, BfmForestCamera *camera);

static int damage_number(BfmSteamKnight *s, const int16_t point[3], uint16_t amount)
{
    for (size_t i = 0; i < BFM_STEAM_ARMOR_PLATES; ++i) {
        if (s->numbers[i].digits.amount) continue;
        if (!bfm_forest_damage_digits_start_data(&s->numbers[i].digits, s->damage_palette,
                                                point, amount)) return 0;
        s->numbers[i].delay = 2;
        break;
    }
    return 1;
}

static int plate_contact(BfmSteamKnight *s, uint16_t amount)
{
    s->hit_sound = 1;
    if (s->hit.change == BFM_STEAM_INNER_REMOVED) {
        s->pending_kneel_flags = s->hit.body_flags_set;
        s->pending_kneel_joint = s->hit.joint;
    }
    if (s->hit.before > s->hit.after)
        bfm_steam_knight_machine_flash_armor(s->machine, s->hit.joint);
    return damage_number(s, s->hit.point, amount);
}

static int core_contact(BfmSteamKnight *s, const BfmSteamKnightHeroStrike *strike,
                        uint16_t amount)
{
    if (s->body_stage != 11 || s->weak_stage == 3 || s->recovery_left) return 1;
    BfmSteamKnightCoreState body = {s->round, s->body_stage, s->weak_stage,
                                    s->battle_flags, (int)s->core_hp, (int)s->core_max};
    BfmForestTransform attachment;
    unsigned held_updates = s->knee_age > 30 ? s->knee_age - 30 : 0;
    unsigned extension = s->weak_stage == 2 ?
        bfm_steam_knight_core_kneel_extension(held_updates, s->weak_age) : 0;
    if (!bfm_steam_knight_core_transform(s->machine, s->weak_stage == 2,
                                         extension, &attachment)) return 0;
    BfmSteamKnightCoreHit hit;
    if (!bfm_steam_knight_core_strike(&body, &s->core_struck, &s->core_boxes,
            &attachment, strike->from, strike->ends, strike->count, amount, &hit)) return 1;

    s->hit_sound = 1;
    if (hit.response == BFM_STEAM_CAP_STRUCK) {
        s->weak_stage = 3;
        s->weak_age = 0;
        return 1;
    }
    s->core_hp = (unsigned)hit.hp_after;
    s->battle_flags = (s->battle_flags & ~hit.clear_flags) | hit.set_flags;
    if (hit.response == BFM_STEAM_CORE_DEPLETED)
        s->body_stage = 10;
    return damage_number(s, hit.contact.point, amount);
}

static int duel_tick(BfmSteamKnight *s, int x, int y, uint16_t pressed, uint16_t held)
{
    BfmSteamKnightEvent *e = &s->event;
    ++e->age;
    ++e->stage_age;
    bfm_forest_voice_step(s->voice);
    bfm_steam_knight_machine_present(s->machine, e->stage_age);
    if (s->weak_stage == 3 && ++s->weak_age == 47) {
        s->weak_stage = 2;
        s->weak_age = 0;
        s->battle_flags |= 2;
    } else if (s->weak_stage == 2 && s->weak_age < 47) {
        ++s->weak_age;
    }
    if (s->body_stage == 11 && s->knee_age < 48) ++s->knee_age;
    if (s->recovery_left && !--s->recovery_left) s->body_stage = 1;
    for (size_t i = 0; i < BFM_STEAM_ARMOR_PLATES; ++i) {
        if (s->numbers[i].delay) --s->numbers[i].delay;
        else if (s->numbers[i].digits.amount) bfm_forest_damage_digits_advance(&s->numbers[i].digits);
    }
    BfmForestCamera camera;
    hall_camera(s, &camera);
    BfmSteamKnightHeroStrike strike;
    if (!bfm_steam_hero_tick(s->hero, &camera, x, y, pressed, held, &strike)) return -1;
    if (strike.issued) {
        bfm_steam_knight_armor_begin_swing(&s->armor);
        s->core_struck = 0;
        s->swing_sound = 1;
    }
    if (strike.roll_strength) {
        uint16_t amount;
        if (!bfm_forest_fusion_strength(&s->power, &e->run->player, strike.power_class,
                bfm_gameplay_story_flag(e->run, 0x81), bfm_gameplay_random(e->run), &amount)) return -1;
        if (strike.count && bfm_steam_knight_armor_strike(&s->armor, s->machine,
                strike.from, strike.ends, strike.count, strike.type, amount, &s->hit) &&
            !plate_contact(s, amount)) return -1;
        if (strike.count && !core_contact(s, &strike, amount)) return -1;
    }
    if (s->body_stage == 11 && !s->recovery_left && (s->battle_flags & 0x80000) &&
        !(s->battle_flags & 0x60000) && bfm_steam_knight_machine_recover(s->machine))
        s->recovery_left = 16;
    if (s->pending_kneel_joint && s->body_stage == 1 &&
        bfm_steam_knight_machine_inner_broken(s->machine, s->pending_kneel_joint)) {
        s->body_stage = 11;
        s->knee_age = 0;
        s->battle_flags |= s->pending_kneel_flags;
        s->pending_kneel_flags = s->pending_kneel_joint = 0;
    }
    return 1;
}

static int presentation_tick(BfmSteamKnight *s, uint16_t pressed, uint16_t held,
                             int32_t initial_walk_velocity)
{
    BfmSteamKnightEvent *e = &s->event;
    if (s->ring_started) {
        BfmSteamFlameRay rays[BFM_ROTATION_RAYS];
        size_t count = 0;
        if (e->stage == BFM_STEAM_ROTATING)
            count = bfm_lumina_rotation_rays(&s->rotation, e->stage_age + 1, e->position, e->heading, rays);
        bfm_steam_knight_flames_tick(&s->flames, rays, count);
        if (!bfm_steam_weapons_tick(&s->weapons, count ? &rays[count - 1] : NULL)) return -1;
    }
    int advanced = bfm_steam_event_tick(e, pressed, held, bfm_steam_knight_flames_remaining(&s->flames));
    if (advanced <= 0) return advanced;
    if (e->stage >= BFM_STEAM_BREACH && e->stage < BFM_STEAM_DUEL) {
        if (e->stage == BFM_STEAM_BREACH && e->stage_age == 8) {
            if (!bfm_steam_knight_field_open_roof(s->field)) return -1;
            bfm_steam_knight_machine_reveal(s->machine);
        }
        bfm_steam_knight_machine_present(s->machine, e->landing_age);
    } else if (e->stage == BFM_STEAM_DUEL) {
        size_t bytes;
        const uint8_t *scene = bfm_steam_knight_field_scene(s->field, &bytes);
        s->hero = bfm_steam_hero_open(s->disc, scene, bytes, e->position, e->heading, initial_walk_velocity);
        if (!s->hero) return -1;
        bfm_steam_knight_machine_handover(s->machine);
    }
    return 1;
}

int bfm_steam_knight_tick(BfmSteamKnight *s, int x, int y, uint16_t pressed, uint16_t held,
                          int32_t initial_walk_velocity)
{
    if (!s) return -1;
    BfmSteamKnightEvent *e = &s->event;
    s->swing_sound = s->hit_sound = 0;
    int advanced = s->hero ? duel_tick(s, x, y, pressed, held)
                           : presentation_tick(s, pressed, held, initial_walk_velocity);
    if (advanced <= 0) return advanced;
    if (!s->ring_started && bfm_steam_event_ring_started(e)) {
        int16_t center[3] = {e->position[0], e->position[1], (int16_t)(e->position[2] - 20)};
        bfm_steam_knight_flames_start(&s->flames, center);
        s->ring_started = 1;
    }
    if (bfm_steam_event_hud(e)) {
        if (!s->hud_shown) {
            bfm_forest_hud_show(s->hud);
            s->hud_shown = 1;
        }
        bfm_forest_hud_gauges(s->hud, e->run->player.record[0x47], -1);
        bfm_forest_hud_submit_points(s->hud, &e->run->player);
        bfm_forest_hud_tick(s->hud);
    }
    return 1;
}

int bfm_steam_knight_swing_sound(const BfmSteamKnight *s) { return s && s->swing_sound; }
int bfm_steam_knight_hit_sound(const BfmSteamKnight *s) { return s && s->hit_sound; }

int bfm_steam_knight_control(const BfmSteamKnight *s)
{
    return s && (s->hero || s->event.stage == BFM_STEAM_READY || s->event.stage == BFM_STEAM_ROTATING);
}

uint16_t bfm_steam_knight_room(const BfmSteamKnight *s)
{
    return s && s->event.age >= SELECT_HALL ? 0x300e : 0x300d;
}

unsigned bfm_steam_knight_shade(const BfmSteamKnight *s)
{
    return s ? bfm_steam_event_shade(&s->event) : 31;
}

int bfm_steam_knight_hero(const BfmSteamKnight *s, int16_t feet[3], int16_t *heading,
                          int *running, unsigned *motion_tick)
{
    if (!s || s->event.age < PLACE_HERO || !feet || !heading || !running || !motion_tick)
        return 0;
    if (s->hero) {
        const BfmSteamKnightHeroSample *sample = bfm_steam_hero_sample(s->hero);
        memcpy(feet, sample->feet, sizeof sample->feet);
        *heading = sample->heading;
        *motion_tick = sample->elapsed;
        *running = sample->motion != 0;
        return 1;
    }
    memcpy(feet, s->event.position, sizeof s->event.position);
    *heading = s->event.heading;
    *running = s->event.stage == BFM_STEAM_ENTER_HALL ||
               (s->event.stage == BFM_STEAM_APPROACH && !s->event.wait) ||
               (s->event.stage == BFM_STEAM_CHASE && s->event.stage_age >= 3 && feet[2] < -3840) ||
               (s->event.stage == BFM_STEAM_CHALLENGE && feet[2] < -3456);
    if (s->event.stage == BFM_STEAM_ROTATING)
        bfm_lumina_rotation_pose(&s->rotation, s->event.stage_age, s->event.heading, heading);
    *motion_tick = s->event.stage_age;
    return 1;
}

uint32_t bfm_steam_knight_body(const BfmSteamKnight *s)
{
    if (!s) return 0;
    if (s->hero) return bfm_steam_hero_sample(s->hero)->body;
    if (s->event.shot == BFM_STEAM_REPLY_CAMERA)
        return s->rotation.keys[s->rotation.held].body;
    if (s->event.stage != BFM_STEAM_ROTATING) return 0;
    int16_t heading;
    const BfmLuminaRotationKey *key = bfm_lumina_rotation_pose(&s->rotation,
        s->event.stage_age, s->event.heading, &heading);
    return key ? key->body : 0;
}

static void hall_camera(const BfmSteamKnight *s, BfmForestCamera *camera)
{
    static void (*const views[])(BfmForestCamera *) = {
        [BFM_STEAM_ENTRY_CAMERA] = bfm_steam_knight_entrance_camera,
        [BFM_STEAM_CAPTOR_CAMERA] = bfm_steam_knight_hostage_camera,
        [BFM_STEAM_HALL_CAMERA] = bfm_steam_knight_hostage_camera,
        [BFM_STEAM_CAPTOR_LOW_CAMERA] = bfm_steam_knight_captor_close_camera,
        [BFM_STEAM_FIRE_CAMERA] = bfm_steam_knight_fire_reverse_camera,
        [BFM_STEAM_PRINCESS_CAMERA] = bfm_steam_knight_princess_close_camera,
        [BFM_STEAM_TUTORIAL_CAMERA] = bfm_steam_knight_charge_camera,
        [BFM_STEAM_REPLY_CAMERA] = bfm_steam_knight_postfire_camera,
        [BFM_STEAM_CARPET_CAMERA] = bfm_steam_knight_hall_camera,
        [BFM_STEAM_ROOF_CAMERA] = bfm_steam_knight_roof_camera,
        [BFM_STEAM_MACHINE_CAMERA] = bfm_steam_knight_machine_camera,
        [BFM_STEAM_DUEL_CAMERA] = bfm_steam_knight_duel_camera
    };
    if (bfm_steam_event_title(&s->event)) {
        unsigned ground_age = s->event.landing_age > 24 ? s->event.landing_age - 24 : 0;
        bfm_steam_knight_title_camera(camera, ground_age * 2);
    } else if (s->event.stage == BFM_STEAM_DUEL && !s->event.stage_age)
        bfm_steam_knight_machine_camera(camera);
    else
        views[s->event.shot](camera);
}

int bfm_steam_knight_draw(BfmSteamKnight *s, const BfmForestPose *pose,
    const BfmForestTransform *root, uint32_t visible, const int8_t fusion[2][3],
    const int8_t lumina[2][3], uint32_t pixels[240][320])
{
    if (!s || !pixels) return 0;
    BfmForestCamera camera;
    hall_camera(s, &camera);
    BfmForestActor actors[2];
    BfmForestPose actor_poses[2];
    BfmForestTransform actor_roots[2];
    /* As Musashi approaches, the pair retreat one carpet tile. Their feet
     * move during the same scripted approach as the player. */
    int approach = s->event.position[2] + 4352;
    if (approach < 0) approach = 0;
    if (approach > 192) approach = 192;
    int16_t z = (int16_t)(-3968 + approach * 256 / 192);
    int16_t floor_positions[][3] = {{-80,-2050,z}, {-192,-2050,z}};
    if (s->event.stage == BFM_STEAM_SILENCE && s->event.stage_age >= 21) {
        unsigned recoil = s->event.stage_age - 21;
        floor_positions[1][2] -= (int16_t)(recoil >= 8 ? 32 : 4 * recoil);
    } else if (s->event.stage > BFM_STEAM_SILENCE) {
        floor_positions[1][2] -= 32;
    }
    size_t actor_count = bfm_steam_event_show_captors(&s->event) ? 2 : 0;
    for (size_t i = 0; i < actor_count; ++i) {
        bfm_forest_actor_root(0, floor_positions[i], &actor_roots[i]);
        unsigned clip = 0, age = s->event.age;
        if (i == BFM_STEAM_ROOTRICK)
            bfm_steam_event_captor_pose(&s->event, &clip, &age);
        else if (s->event.stage == BFM_STEAM_HOSTAGE_PRAISE) {
            clip = 1;
            age = s->event.stage_age;
        } else if (s->event.stage == BFM_STEAM_SILENCE && s->event.stage_age >= 21) {
            clip = 2;
            age = s->event.stage_age - 21;
        } else if (s->event.shot == BFM_STEAM_PRINCESS_CAMERA) {
            clip = 1;
        }
        if (!bfm_steam_knight_hostage_actor(s->hostages, i, clip, age,
                                             &actor_roots[i], &actor_poses[i], &actors[i]))
            return 0;
        if (i == BFM_STEAM_FILLET && clip == 2 &&
            !bfm_steam_knight_fillet_close_eyes(s->hostages, &actors[i])) return 0;
    }
    if (!actor_count)
        actor_count = bfm_steam_knight_machine_actors(s->machine, actors, 2);
    BfmForestDustCard cards[BFM_STEAM_ENTRY_LIGHT_CARDS + BFM_STEAM_FLAMES + BFM_STEAM_WEAPON_CARDS
                            + BFM_STEAM_ARMOR_PLATES * BFM_FOREST_DAMAGE_DIGIT_CARDS];
    size_t count = s->event.shot == BFM_STEAM_ENTRY_CAMERA
                 ? bfm_steam_entry_light(&camera, s->event.age, cards) : 0;
    count += bfm_steam_knight_flames_project(&s->flames, &camera, cards + count);
    BfmForestBladeFrame blade;
    int has_blade;
    count += bfm_steam_weapons_project(&s->weapons, &camera, root, visible, fusion, lumina,
                                       s->event.age, &blade, &has_blade, cards + count);
    for (size_t i = 0; i < BFM_STEAM_ARMOR_PLATES; ++i)
        count += bfm_forest_damage_digits_project(&s->numbers[i].digits, &camera, cards + count);
    for (size_t i = 0; i < count; ++i) cards[i].slot = (unsigned)i;
    if (!bfm_steam_knight_field_draw(s->field, &camera, pose, root, visible,
            has_blade ? &s->weapons.fusion : NULL, has_blade ? &blade : NULL, s->event.age & 1,
            actors, actor_count, cards, count, pixels)) return 0;
    if (s->hud_shown && bfm_steam_event_hud(&s->event))
        bfm_forest_hud_draw(s->hud, &s->event.run->player, pixels);
    const uint16_t (*tiles)[1024] = bfm_forest_terrain_tiles(bfm_steam_knight_field_terrain(s->field));
    if (bfm_steam_event_title(&s->event)) bfm_steam_knight_boss_title(tiles, pixels);
    if (s->event.stage == BFM_STEAM_DUEL)
        bfm_steam_knight_boss_status(&s->boss_hud, tiles, s->core_hp, s->event.stage_age, pixels);
    if (s->event.message) {
        const BfmForestMessageFrame *frame = bfm_forest_message_frame(s->event.message);
        bfm_bedroom_message_art_draw_side(s->art, frame, 160, 0, pixels);
    }
    return 1;
}

ForestVoice *bfm_steam_knight_voice(BfmSteamKnight *s, unsigned *generation)
{
    if (!s || !generation) return NULL;
    *generation = s->event.voice_generation;
    return s->voice;
}
