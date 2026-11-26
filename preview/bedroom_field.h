/* SPDX-License-Identifier: MIT */
#ifndef BFM_BEDROOM_FIELD_H
#define BFM_BEDROOM_FIELD_H
#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

enum { BFM_FIELD_SCENE_CAPACITY = 1 << 20 };
size_t bfm_read_field_scene(const MusashiDiscMedia *disc, unsigned member,
                            uint8_t scene[BFM_FIELD_SCENE_CAPACITY]);

typedef struct BfmBedroomField BfmBedroomField;
typedef struct {
    int axes[3][3], translation[3], focal;
    int eye[3], target[3];
} BfmBedroomView;

/* The room address and event actor origin belong to the active mounted room.
 * The event camera operands are read by the caller from its mounted script. */
BfmBedroomField *bfm_bedroom_field_load(const uint8_t *scene, size_t bytes,
                                        uint32_t room_address);
void bfm_bedroom_field_free(BfmBedroomField *field);
int bfm_bedroom_field_camera(BfmBedroomField *field, const uint8_t *scene,
                             size_t bytes, uint32_t camera_address,
                             const int actor_origin[3], int immediate);
int bfm_bedroom_field_orbit(BfmBedroomField *field, const int target[3], int yaw,
                            int distance, int pitch, int immediate, unsigned divisor);
void bfm_bedroom_field_viewpoint(BfmBedroomField *field, const int eye[3], const int target[3],
                                 int immediate, unsigned divisor);
void bfm_bedroom_field_step(BfmBedroomField *field);
const BfmBedroomView *bfm_bedroom_field_view(const BfmBedroomField *field);
int bfm_bedroom_project(const BfmBedroomView *view, const int16_t model[3], int xy_depth[3]);
void bfm_bedroom_field_draw(BfmBedroomField *field, const uint16_t tiles[512][1024],
                            uint32_t *pixels, void (*draw_to)(void *, int), void *user);
#endif
