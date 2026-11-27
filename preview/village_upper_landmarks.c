/* SPDX-License-Identifier: MIT */
#include "village_upper_landmarks.h"
#include "woods_sign_model.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { SIGN_COUNT = 3, FIRST_VARIANT = 6, ROW_BYTES = 20 };
static const uint32_t origin = UINT32_C(0x80128158);

typedef struct {
    BfmWoodsSignModel *model;
    const uint8_t *reading;
    size_t length;
} Board;

struct BfmVillageUpperLandmarks {
    Board boards[SIGN_COUNT];
};

static const uint8_t *at(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (!scene || address < origin || length > bytes || address - origin > bytes - length)
        return NULL;
    return scene + (address - origin);
}

void bfm_village_upper_landmarks_free(BfmVillageUpperLandmarks *landmarks)
{
    if (!landmarks) return;
    for (unsigned i = 0; i < SIGN_COUNT; ++i)
        bfm_woods_sign_model_free(landmarks->boards[i].model);
    free(landmarks);
}

static int load_board(Board *board, const uint8_t *scene, size_t bytes, uint32_t row,
                      unsigned variant)
{
    const uint8_t *table = at(scene,bytes,UINT32_C(0x801c2e38) + 4 * variant,4);
    if (!table) return 0;
    const uint8_t *script = at(scene,bytes,u32le(table),24);
    if (!script || u32le(script) != 10 || u32le(script + 8) != 1 ||
        u32le(script + 16) != 2) return 0;
    uint32_t text = u32le(script + 12);
    board->reading = at(scene,bytes,text,1);
    if (!board->reading || board->reading[0] != 5) return 0;
    const uint8_t *end = memchr(board->reading,0,bytes - (text - origin));
    if (!end) return 0;
    board->length = (size_t)(end - board->reading) + 1;
    board->model = bfm_woods_sign_model_from_placement(scene,bytes,row,UINT32_C(0x801c2d70));
    return board->model != NULL;
}

BfmVillageUpperLandmarks *bfm_village_upper_landmarks_open(const uint8_t *scene, size_t bytes,
                                                           uint32_t room)
{
    if (room != UINT32_C(0x8018c794)) return NULL;
    const uint8_t *descriptor = at(scene,bytes,room,0xbc);
    if (!descriptor) return NULL;
    uint32_t first = u32le(descriptor + 0xb4), end = u32le(descriptor + 0xb8);
    if (end <= first || (end - first) % ROW_BYTES) return NULL;
    const uint8_t *rows = at(scene,bytes,first,end - first);
    if (!rows) return NULL;
    BfmVillageUpperLandmarks *landmarks = calloc(1,sizeof *landmarks);
    if (!landmarks) return NULL;
    unsigned loaded = 0;
    for (uint32_t offset = 0; offset < end - first; offset += ROW_BYTES) {
        const uint8_t *row = rows + offset;
        unsigned kind = u16le(row + 6);
        if (!kind) break;
        if (kind != 492) continue;
        unsigned variant = u16le(row + 8);
        if (variant < FIRST_VARIANT || variant >= FIRST_VARIANT + SIGN_COUNT) {
            bfm_village_upper_landmarks_free(landmarks);
            return NULL;
        }
        Board *board = landmarks->boards + (variant - FIRST_VARIANT);
        if (board->model || !load_board(board,scene,bytes,first + offset,variant)) {
            bfm_village_upper_landmarks_free(landmarks);
            return NULL;
        }
        ++loaded;
    }
    if (loaded == SIGN_COUNT) return landmarks;
    bfm_village_upper_landmarks_free(landmarks);
    return NULL;
}

int bfm_village_upper_landmarks_prepare(BfmVillageUpperLandmarks *landmarks,
        const BfmBedroomView *view, const uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!landmarks) return 0;
    for (unsigned i = 0; i < SIGN_COUNT; ++i)
        if (!bfm_woods_sign_model_prepare(landmarks->boards[i].model,view,tiles,pixels)) return 0;
    return 1;
}

int bfm_village_upper_landmarks_next_depth(const BfmVillageUpperLandmarks *landmarks)
{
    int farthest = INT_MIN;
    if (landmarks) for (unsigned i = 0; i < SIGN_COUNT; ++i) {
        int depth = bfm_woods_sign_model_next_depth(landmarks->boards[i].model);
        if (depth > farthest) farthest = depth;
    }
    return farthest;
}

void bfm_village_upper_landmarks_draw_to(BfmVillageUpperLandmarks *landmarks, int depth)
{
    if (!landmarks) return;
    int bucket;
    while ((bucket = bfm_village_upper_landmarks_next_depth(landmarks)) != INT_MIN && bucket >= depth)
        for (unsigned i = 0; i < SIGN_COUNT; ++i)
            bfm_woods_sign_model_draw_to(landmarks->boards[i].model,bucket);
}

int bfm_village_upper_landmarks_slide(const BfmVillageUpperLandmarks *landmarks,
                                      const int16_t previous[3], int16_t endpoint[3])
{
    int contact = 0;
    if (landmarks) for (unsigned i = 0; i < SIGN_COUNT; ++i)
        contact |= bfm_woods_sign_model_slide(landmarks->boards[i].model,previous,endpoint);
    return contact;
}

BfmForestMessage *bfm_village_upper_landmarks_read(const BfmVillageUpperLandmarks *landmarks,
                                                    const int16_t hero[3], int square_pressed,
                                                    int16_t point[3])
{
    if (!landmarks || !hero || !square_pressed) return NULL;
    for (unsigned i = 0; i < SIGN_COUNT; ++i) {
        const Board *board = landmarks->boards + i;
        const int16_t *post = bfm_woods_sign_model_position(board->model);
        int64_t squared = 0;
        for (unsigned axis = 0; axis < 3; ++axis) {
            int64_t delta = (int)hero[axis] - post[axis];
            squared += delta * delta;
        }
        if (squared <= 64 * 64) {
            if (point) memcpy(point,post,3 * sizeof *point);
            return bfm_field_message_borrow(board->reading,board->length);
        }
    }
    return NULL;
}
