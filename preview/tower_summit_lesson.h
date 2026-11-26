/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_LESSON_H
#define BFM_TOWER_SUMMIT_LESSON_H
#include "forest_field.h"
#include "forest_message_art.h"
#include "forest_voice.h"

typedef struct {
    int16_t walk_x, walk_z, turn_yaw;
    BfmForestMessage *page;
    const uint8_t *reaction, *notice;
    size_t reaction_bytes, notice_bytes;
    int32_t eye[3], aim[3], plate_eye[3], plate_aim[3];
    unsigned phase, age, wait, initial_wait, final_wait;
    int speaker_x;
} BfmTowerSummitLesson;

enum { BFM_SUMMIT_LESSON_ERROR, BFM_SUMMIT_LESSON_LOCKED, BFM_SUMMIT_LESSON_RELEASED };

/* Begin once when the plate event accepts the player. The room must outlive
 * the lesson. The caller supplies the ordinary summit eye from this update. */
int bfm_tower_summit_lesson_begin(BfmTowerSummitLesson *lesson, const BfmForestField *room,
                                  const int16_t player[3], const int32_t eye[3]);
/* Drive the ordinary scripted approach and facing at the returned walk_x,
 * walk_z and turn_yaw. Pass settled once that turn finishes. Advance shared
 * voice transport first; confirm and reveal are fresh focused-button edges. */
int bfm_tower_summit_lesson_tick(BfmTowerSummitLesson *lesson, ForestVoice *voice,
                                 unsigned *generation, const int16_t player[3],
                                 int settled, int confirm, int reveal);
int bfm_tower_summit_lesson_camera(const BfmTowerSummitLesson *lesson,
                                   BfmForestCamera *camera, int32_t eye[3]);
void bfm_tower_summit_lesson_draw(const BfmTowerSummitLesson *lesson,
                                 const BfmForestMessageArt *art, uint32_t pixels[240][320]);
void bfm_tower_summit_lesson_close(BfmTowerSummitLesson *lesson);
#endif
