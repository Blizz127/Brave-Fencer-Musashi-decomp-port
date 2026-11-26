/* SPDX-License-Identifier: MIT */
#include "steam_knight.h"
#include "steam_knight_event.h"
#include "steam_knight_cast.h"
#include "steam_knight_rotation.h"
#include "steam_knight_effects.h"
#include "steam_knight_entry_light.h"
#include "forest_hud.h"
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
    s->field = bfm_steam_knight_field_open(disc);
    if (!s->field) goto fail;
    size_t size;
    const uint8_t *scene = bfm_steam_knight_field_scene(s->field, &size);
    const RoomTransferDescriptor *entry = bfm_steam_knight_field_entrance(s->field);
    const uint16_t (*tiles)[1024] = bfm_forest_terrain_tiles(bfm_steam_knight_field_terrain(s->field));
    s->art = bfm_castle_message_art_load(disc, scene, size, tiles, 0x68e58);
    s->hostages = bfm_steam_knight_hostages_load(disc, size);
    s->hud = bfm_steam_knight_hud_load(disc, scene, size, bfm_steam_knight_field_terrain(s->field));
    s->voice = bfm_forest_voice_load(disc);
    if (!s->art || !s->hostages || !s->hud || !bfm_lumina_rotation_load(disc, &s->rotation) ||
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
    bfm_steam_knight_hostages_free(s->hostages);
    bfm_forest_hud_free(s->hud);
    bfm_forest_voice_free(s->voice);
    bfm_forest_message_art_free(s->art);
    bfm_steam_knight_field_free(s->field);
    free(s);
}

int bfm_steam_knight_tick(BfmSteamKnight *s, uint16_t pressed, uint16_t held)
{
    if (!s) return -1;
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

int bfm_steam_knight_control(const BfmSteamKnight *s)
{
    return s && (s->event.stage == BFM_STEAM_READY || s->event.stage == BFM_STEAM_ROTATING);
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
    memcpy(feet, s->event.position, sizeof s->event.position);
    *heading = s->event.heading;
    *running = s->event.stage == BFM_STEAM_ENTER_HALL ||
               (s->event.stage == BFM_STEAM_APPROACH && !s->event.wait);
    if (s->event.stage == BFM_STEAM_ROTATING)
        bfm_lumina_rotation_pose(&s->rotation, s->event.stage_age, s->event.heading, heading);
    *motion_tick = s->event.stage_age;
    return 1;
}

uint32_t bfm_steam_knight_body(const BfmSteamKnight *s)
{
    if (!s) return 0;
    if (s->event.shot == BFM_STEAM_REPLY_CAMERA)
        return s->rotation.keys[s->rotation.held].body;
    if (s->event.stage != BFM_STEAM_ROTATING) return 0;
    int16_t heading;
    const BfmLuminaRotationKey *key = bfm_lumina_rotation_pose(&s->rotation,
        s->event.stage_age, s->event.heading, &heading);
    return key ? key->body : 0;
}

int bfm_steam_knight_draw(BfmSteamKnight *s, const BfmForestPose *pose,
    const BfmForestTransform *root, uint32_t visible, const int8_t fusion[2][3],
    const int8_t lumina[2][3], uint32_t pixels[240][320])
{
    if (!s || !pixels) return 0;
    static void (*const views[])(BfmForestCamera *) = {
        [BFM_STEAM_ENTRY_CAMERA] = bfm_steam_knight_entrance_camera,
        [BFM_STEAM_CAPTOR_CAMERA] = bfm_steam_knight_hostage_camera,
        [BFM_STEAM_HALL_CAMERA] = bfm_steam_knight_hostage_camera,
        [BFM_STEAM_CAPTOR_LOW_CAMERA] = bfm_steam_knight_captor_close_camera,
        [BFM_STEAM_FIRE_CAMERA] = bfm_steam_knight_fire_reverse_camera,
        [BFM_STEAM_PRINCESS_CAMERA] = bfm_steam_knight_princess_close_camera,
        [BFM_STEAM_TUTORIAL_CAMERA] = bfm_steam_knight_charge_camera,
        [BFM_STEAM_REPLY_CAMERA] = bfm_steam_knight_postfire_camera
    };
    BfmForestCamera camera;
    views[s->event.shot](&camera);
    BfmForestActor actors[2];
    BfmForestPose actor_poses[2];
    BfmForestTransform actor_roots[2];
    /* As Musashi approaches, the pair retreat one carpet tile. Their feet
     * move during the same scripted approach as the player. */
    int approach = s->event.position[2] + 4352;
    if (approach < 0) approach = 0;
    if (approach > 192) approach = 192;
    int16_t z = (int16_t)(-3968 + approach * 256 / 192);
    const int16_t floor_positions[][3] = {{-80,-2050,z}, {-192,-2050,z}};
    for (unsigned i = 0; i < 2; ++i) {
        bfm_forest_actor_root(0, floor_positions[i], &actor_roots[i]);
        unsigned clip = 0, age = s->event.age;
        if (i == BFM_STEAM_ROOTRICK)
            bfm_steam_event_captor_pose(&s->event, &clip, &age);
        else if (s->event.shot == BFM_STEAM_PRINCESS_CAMERA)
            clip = 1;
        if (!bfm_steam_knight_hostage_actor(s->hostages, i, clip, age,
                                             &actor_roots[i], &actor_poses[i], &actors[i]))
            return 0;
    }
    BfmForestDustCard cards[BFM_STEAM_ENTRY_LIGHT_CARDS + BFM_STEAM_FLAMES + BFM_STEAM_WEAPON_CARDS];
    size_t count = s->event.shot == BFM_STEAM_ENTRY_CAMERA
                 ? bfm_steam_entry_light(&camera, s->event.age, cards) : 0;
    count += bfm_steam_knight_flames_project(&s->flames, &camera, cards + count);
    BfmForestBladeFrame blade;
    int has_blade;
    count += bfm_steam_weapons_project(&s->weapons, &camera, root, visible, fusion, lumina,
                                       s->event.age, &blade, &has_blade, cards + count);
    for (size_t i = 0; i < count; ++i) cards[i].slot = (unsigned)i;
    if (!bfm_steam_knight_field_draw(s->field, &camera, pose, root, visible,
            has_blade ? &s->weapons.fusion : NULL, has_blade ? &blade : NULL, s->event.age & 1,
            actors, 2, cards, count, pixels)) return 0;
    if (s->hud_shown && bfm_steam_event_hud(&s->event))
        bfm_forest_hud_draw(s->hud, &s->event.run->player, pixels);
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
