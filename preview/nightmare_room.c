/* SPDX-License-Identifier: MIT */
#include "nightmare_room.h"
#include "nightmare_field.h"
#include "nightmare_departure.h"
#include "forest_message_art.h"
#include "nightmare_wake.h"
#include "scene_payload.h"
#include "scene_tiles.h"
#include "village_field.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum WakePhase { SLEEPING, STARTLED, SETTLING, SCRIPT_DELAY, CLOSE_SHOT };

struct BfmNightmareRoom {
    uint8_t *scene, *main;
    const MusashiDiscMedia *disc;
    uint32_t room_address;
    const BfmNightmareLayout *layout;
    BfmNightmareDeparture *departure;
    BfmForestMessageArt *ink;
    ForestVoice *voice;
    unsigned generation;
    const uint8_t *exit;
    BfmBedroomMotion *motion;
    BfmBedroomPose pose;
    BfmBedroomField *field;
    BfmBedroomFigure *hero;
    BfmBedroomRoot root;
    BfmNightmareWake wake;
    size_t scene_bytes;
    uint16_t tiles[512][1024];
    uint8_t flags[64];
    unsigned update, remaining, script_delay;
    enum WakePhase phase;
};

void bfm_nightmare_room_free(BfmNightmareRoom *room)
{
    if (!room) return;
    bfm_nightmare_departure_free(room->departure);
    bfm_forest_message_art_free(room->ink);
    bfm_forest_voice_free(room->voice);
    bfm_bedroom_motion_free(room->motion);
    bfm_bedroom_figure_free(room->hero);
    bfm_bedroom_field_free(room->field);
    free(room->main);
    free(room->scene);
    free(room);
}

BfmNightmareRoom *bfm_nightmare_room_open(const MusashiDiscMedia *disc,
                                          const BfmBedroomEntry *entry)
{
    const BfmNightmareLayout *layout = bfm_nightmare_layout(entry);
    if (!disc || !layout) return NULL;
    BfmNightmareRoom *room = calloc(1, sizeof *room);
    if (!room) return NULL;
    size_t bytes = 0, main_bytes;
    room->layout = layout;
    room->scene = bfm_scene_unpack_pac0(disc, "SC03.CD", entry->scene_member,
                                        BFM_FIELD_SCENE_CAPACITY, BFM_FIELD_SCENE_CAPACITY, &bytes);
    uint32_t address;
    room->field = bfm_nightmare_field_open(room->scene, bytes, entry, &address);
    if (!room->field) goto fail;
    BfmBedroomTravel *travel = bfm_bedroom_travel_open(room->scene, bytes, address, 0);
    if (!travel) goto fail;
    room->root = *bfm_bedroom_travel_root(travel);
    bfm_bedroom_travel_free(travel);

    room->main = bfm_bedroom_player_data(disc, &main_bytes);
    room->motion = bfm_bedroom_motion_open(room->scene, bytes, room->main, main_bytes, address);
    room->hero = bfm_bedroom_figure_load(disc, room->scene, bytes, address,
                                          layout->figure, layout->face);
    /* The grounded player starts with the ordinary face before the field
     * script takes over the body. The sleeping clip leaves this face alone. */
    if (!room->motion || !room->hero ||
        !bfm_scene_read_tiles(disc, "MAIN.CD", 10, room->tiles) ||
        !bfm_scene_read_tiles(disc, "SC03.CD", entry->scene_member, room->tiles) ||
        !bfm_bedroom_motion_reenter_stance(room->motion, 0) ||
        !bfm_bedroom_motion_step(room->motion, BFM_BEDROOM_NIGHTMARE_SLEEP, &room->pose)) goto fail;
    memcpy(room->flags, entry->state.flags, sizeof room->flags);
    size_t script = layout->script - 0x80128158u;
    if (bytes < script + 16 || u32le(room->scene + script + 4) != UINT32_C(0x8016f6e0) ||
        u32le(room->scene + script + 12) != UINT32_C(0x8017e950)) goto fail;
    room->script_delay = u32le(room->scene + script + 8);
    room->disc = disc;
    room->room_address = address;
    room->scene_bytes = bytes;
    room->update = 1;
    return room;
fail:
    bfm_nightmare_room_free(room);
    return NULL;
}

BfmNightmareStep bfm_nightmare_room_step(BfmNightmareRoom *room)
{
    if (!room) return BFM_NIGHTMARE_ERROR;
    /* The wake flash starts on the last poll of the sleeping actor. The
     * following update resets its scale, starts a 16-count wait, then the
     * mounted four-count delay runs through update 170. */
    if (room->phase == CLOSE_SHOT) return BFM_NIGHTMARE_CLOSE_SHOT;
    BfmNightmareWake wake = room->wake;
    if (room->update == 146) {
        if (!bfm_nightmare_wake_start(&wake, room->scene, room->scene_bytes,
                                      room->layout->flash, room->root.xyz))
            return BFM_NIGHTMARE_ERROR;
    } else {
        bfm_nightmare_wake_step(&wake);
    }
    BfmBedroomAction action = room->update == 129 ? BFM_BEDROOM_NIGHTMARE_HOLD : BFM_BEDROOM_NONE;
    if (!bfm_bedroom_motion_step(room->motion, action, &room->pose)) return BFM_NIGHTMARE_ERROR;
    switch (room->phase) {
    case SLEEPING:
        if (room->update == 146) room->phase = STARTLED;
        break;
    case STARTLED:
        room->phase = SETTLING;
        room->remaining = 16;
        break;
    case SETTLING:
    case SCRIPT_DELAY:
        if (room->remaining) --room->remaining;
        else if (room->phase == SETTLING) {
            room->phase = SCRIPT_DELAY;
            room->remaining = room->script_delay;
        } else room->phase = CLOSE_SHOT;
        break;
    case CLOSE_SHOT:
        break;
    }
    room->wake = wake;
    ++room->update;
    return BFM_NIGHTMARE_ASLEEP;
}

static int begin_departure(BfmNightmareRoom *room)
{
    BfmNightmareDeparture *departure = bfm_nightmare_departure_open(room->scene, room->scene_bytes,
                room->room_address, room->layout, room->motion, room->field, &room->root, &room->pose);
    if (!departure) return 0;
    BfmForestMessageArt *ink = bfm_castle_message_art_load(room->disc, room->scene, room->scene_bytes,
                                      (const uint16_t (*)[1024])room->tiles, room->layout->window - 0x80128158u);
    ForestVoice *voice = ink ? bfm_defeat_voice_load(room->disc) : NULL;
    if (!voice) {
        bfm_forest_message_art_free(ink);
        bfm_nightmare_departure_free(departure);
        return 0;
    }
    room->departure = departure;
    room->ink = ink;
    room->voice = voice;
    return 1;
}

BfmNightmareStep bfm_nightmare_room_step_field(BfmNightmareRoom *room, int confirm, int reveal,
                                               int other_transfer, BfmNightmareRoomEvent *event)
{
    if (!room || !event) return BFM_NIGHTMARE_ERROR;
    *event = (BfmNightmareRoomEvent){0};
    if (room->phase != CLOSE_SHOT) return bfm_nightmare_room_step(room);
    if (!room->exit) {
        if (!room->departure && !begin_departure(room)) return BFM_NIGHTMARE_ERROR;
        if (!bfm_nightmare_departure_step(room->departure, room->voice, &room->generation,
                                          confirm, reveal, other_transfer, event)) return BFM_NIGHTMARE_ERROR;
        ++room->update;
        room->exit = event->destination;
    }
    event->destination = room->exit;
    return room->exit ? BFM_NIGHTMARE_TRANSFER : BFM_NIGHTMARE_DEPARTING;
}

uint16_t bfm_nightmare_room_number(const BfmNightmareRoom *room)
{
    return room ? room->layout->number : 0;
}

ForestVoice *bfm_nightmare_room_voice(BfmNightmareRoom *room)
{
    return room ? room->voice : NULL;
}

unsigned bfm_nightmare_room_generation(const BfmNightmareRoom *room)
{
    return room ? room->generation : 0;
}

int bfm_nightmare_room_exit_camera(BfmNightmareRoom *room)
{
    if (!room) return 0;
    int target[3] = {room->root.xyz[0], room->root.xyz[1], room->root.xyz[2]};
    return bfm_bedroom_field_set_focal(room->field, 500) &&
           bfm_bedroom_field_orbit(room->field, target, 0, 1500, 568, 1, 1);
}

int bfm_nightmare_room_draw(BfmNightmareRoom *room, uint32_t pixels[240][320])
{
    if (!room || !pixels || !bfm_bedroom_motion_face(room->motion, room->tiles)) return 0;
    const BfmBedroomView *view = bfm_bedroom_field_view(room->field);
    BfmBedroomBody body = {
        .pose = room->pose.pose, .pose_bytes = 256,
        .heading = (int)room->root.yaw, .pitch = (int)room->pose.pitch,
        .face_word = room->pose.face_word, .shadow_visible = room->root.shadow_visible,
        .visible_parts = bfm_village_field_visible(&room->pose, room->flags)
    };
    for (unsigned axis = 0; axis < 3; ++axis) {
        body.position[axis] = room->root.xyz[axis];
        body.surface_normal[axis] = room->root.normal[axis];
        body.shadow_position[axis] = room->root.shadow_xyz[axis];
        body.shadow_normal[axis] = room->root.shadow_normal[axis];
    }
    const int wake_scale[3] = {4224,4224,4224};
    const int *scale = room->phase == STARTLED ? wake_scale : NULL;
    if (!bfm_bedroom_figure_prepare_axes(room->hero, view, &body, BFM_HERO_NORMAL_SCALE,
                                         scale, room->tiles, &pixels[0][0])) return 0;
    BfmForestDustCard card;
    if (bfm_nightmare_wake_card(&room->wake, view, &card) &&
        !bfm_bedroom_figure_add_fusion(room->hero, view, NULL, &card, 1)) return 0;
    for (unsigned y = 0; y < 240; ++y)
        for (unsigned x = 0; x < 320; ++x) pixels[y][x] = UINT32_C(0xff000000);
    bfm_bedroom_field_draw(room->field, (const uint16_t (*)[1024])room->tiles, &pixels[0][0],
                           bfm_bedroom_figure_draw_to, room->hero);
    if (room->ink) {
        const BfmForestMessageFrame *message = bfm_nightmare_departure_message(room->departure);
        bfm_bedroom_message_art_draw_side(room->ink, message, 160, 0, pixels);
    }
    return 1;
}
