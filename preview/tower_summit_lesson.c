/* SPDX-License-Identifier: MIT */
#include "tower_summit_lesson.h"
#include "tower_summit_camera.h"
#include "forest_shore_motion.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { APPROACH = 1, REACTION, NOTICE, AFTER_NOTICE, RETURN_CAMERA, FINISHED };

static int floor_q12(int value)
{
    return value / 4096 - (value % 4096 < 0);
}

int bfm_tower_summit_lesson_begin(BfmTowerSummitLesson *lesson, const BfmForestField *room,
                                  const int16_t player[3], const int32_t eye[3])
{
    if (!lesson || lesson->phase || !room || !player || !eye) return 0;
    const uint8_t *door = bfm_forest_field_entrance(room, 0);
    const uint8_t *event = bfm_forest_room_bytes(room, UINT32_C(0x8018f33c), 80);
    const uint8_t *marker = bfm_forest_room_bytes(room, UINT32_C(0x8018f324), 8);
    const uint8_t *placements = bfm_forest_room_bytes(room, UINT32_C(0x801a7be4), 4);
    if (!door || u16le(door) != 0x300b || !event || !placements || !marker) return 0;
    const uint8_t *shot = bfm_forest_room_bytes(room, u32le(event + 4), 16);
    if (!shot) return 0;
    uint32_t first = u32le(event + 52), second = u32le(event + 60);
    uint32_t end = u32le(placements);
    if (first >= second || second >= end) return 0;
    BfmTowerSummitLesson fresh = {.phase=APPROACH, .reaction_bytes=second-first,
        .notice_bytes=end-second, .walk_x=s16le(event+12), .walk_z=s16le(event+14),
        .initial_wait=u16le(event+36),
        .final_wait=u16le(event+68), .speaker_x=160+s16le(event+44)};
    /* The order faces a point beyond the sword. Body forward is local -Z. */
    fresh.turn_yaw = (int16_t)bfm_forest_shore_bearing(
        fresh.walk_x - s16le(event + 28), fresh.walk_z - s16le(event + 30));
    fresh.reaction = bfm_forest_room_bytes(room, first, fresh.reaction_bytes);
    fresh.notice = bfm_forest_room_bytes(room, second, fresh.notice_bytes);
    if (!fresh.reaction || !fresh.notice) return 0;

    /* The lesson actor owns this camera marker beyond the plate center. */
    int16_t origin[3] = {s16le(marker), s16le(marker+2), s16le(marker+4)};
    BfmForestTransform marker_root, arm;
    bfm_forest_actor_root(u16le(marker+6), origin, &marker_root);
    bfm_forest_actor_pitch_yaw((unsigned)-s16le(shot+12), u16le(shot+8), origin, &arm);
    for (unsigned axis = 0; axis < 3; ++axis) {
        int dot = 0;
        for (unsigned j = 0; j < 3; ++j) dot += marker_root.rotation[axis][j]*s16le(shot+2*j);
        fresh.plate_aim[axis] = origin[axis] + floor_q12(dot);
        fresh.plate_eye[axis] = fresh.plate_aim[axis] + floor_q12(-s16le(shot+10)*arm.rotation[axis][2]);
    }
    memcpy(fresh.eye, eye, sizeof fresh.eye);
    bfm_tower_summit_camera_target(player, fresh.aim);
    *lesson = fresh;
    return 1;
}

static void ease(int32_t current[3], const int32_t goal[3], int divisor)
{
    for (unsigned i = 0; i < 3; ++i) {
        int delta = goal[i] - current[i];
        int scale = divisor;
        while (scale > 1 && delta && abs(delta) < scale) scale /= 2;
        int step = delta / scale;
        current[i] += step;
    }
}

int bfm_tower_summit_lesson_tick(BfmTowerSummitLesson *lesson, ForestVoice *voice,
                                 unsigned *generation, const int16_t player[3],
                                 int settled, int confirm, int reveal)
{
    if (!lesson || !lesson->phase || !player) return BFM_SUMMIT_LESSON_ERROR;
    if (lesson->phase == FINISHED) return BFM_SUMMIT_LESSON_RELEASED;
    if (lesson->phase == RETURN_CAMERA) {
        BfmForestCamera ordinary;
        int32_t eye[3], aim[3];
        if (!bfm_tower_summit_camera(player, &ordinary, eye)) return BFM_SUMMIT_LESSON_ERROR;
        bfm_tower_summit_camera_target(player, aim);
        ease(lesson->eye, eye, 6);
        ease(lesson->aim, aim, 6);
        if (++lesson->wait >= 25) {
            lesson->phase = FINISHED;
            return BFM_SUMMIT_LESSON_RELEASED;
        }
    } else {
        ease(lesson->eye, lesson->plate_eye, 12);
        ease(lesson->aim, lesson->plate_aim, 12);
        if (lesson->phase == APPROACH) {
            /* The quiet interval runs alongside the actor's turn at the marker. */
            if (player[0] == lesson->walk_x && player[2] == lesson->walk_z &&
                lesson->wait < lesson->initial_wait) ++lesson->wait;
            if (settled && lesson->wait == lesson->initial_wait) {
                lesson->page = bfm_field_message_borrow(lesson->reaction, lesson->reaction_bytes);
                if (!lesson->page) return BFM_SUMMIT_LESSON_ERROR;
                lesson->phase = REACTION;
            }
        } else if (lesson->phase == AFTER_NOTICE) {
            if (++lesson->wait >= lesson->final_wait) {
                lesson->phase = RETURN_CAMERA;
                lesson->wait = 0;
            }
        } else {
            if (!voice || !generation) return BFM_SUMMIT_LESSON_ERROR;
            const BfmForestMessageFrame *frame = bfm_forest_voice_page(voice, lesson->page,
                                                                       generation, confirm, reveal);
            if (!frame) return BFM_SUMMIT_LESSON_ERROR;
            if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
                bfm_forest_message_free(lesson->page);
                lesson->page = NULL;
                if (lesson->phase == REACTION) {
                    lesson->page = bfm_field_message_borrow(lesson->notice, lesson->notice_bytes);
                    if (!lesson->page) return BFM_SUMMIT_LESSON_ERROR;
                    lesson->phase = NOTICE;
                } else {
                    lesson->phase = AFTER_NOTICE;
                    lesson->wait = 0;
                }
            }
        }
    }
    ++lesson->age;
    return BFM_SUMMIT_LESSON_LOCKED;
}

int bfm_tower_summit_lesson_camera(const BfmTowerSummitLesson *lesson,
                                   BfmForestCamera *camera, int32_t eye[3])
{
    if (!lesson || !lesson->phase || !camera) return 0;
    int32_t near_aim[3];
    for (unsigned i = 0; i < 3; ++i)
        near_aim[i] = lesson->eye[i] + (lesson->aim[i] - lesson->eye[i]) / 2;
    if (!bfm_forest_camera_look_at(lesson->eye, near_aim, 300, camera)) return 0;
    if (eye) memcpy(eye, lesson->eye, sizeof lesson->eye);
    return 1;
}

void bfm_tower_summit_lesson_draw(const BfmTowerSummitLesson *lesson,
                                 const BfmForestMessageArt *art, uint32_t pixels[240][320])
{
    if (lesson && lesson->page)
        bfm_bedroom_message_art_draw_side(art, bfm_forest_message_frame(lesson->page),
                                         lesson->speaker_x, 0, pixels);
}

void bfm_tower_summit_lesson_close(BfmTowerSummitLesson *lesson)
{
    if (lesson) {
        bfm_forest_message_free(lesson->page);
        *lesson = (BfmTowerSummitLesson){0};
    }
}
