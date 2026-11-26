/* SPDX-License-Identifier: MIT */
#include "village_field.h"

void bfm_village_field_clear(BfmVillageField *field)
{
    bfm_bedroom_field_free(field->field);
    bfm_bedroom_figure_free(field->hero);
    *field = (BfmVillageField){0};
}

/* The two SC03 members place the same face, shadow and equipped-weapon
 * resources at different addresses. Offsets below are relative to the
 * first face texture coordinate table in each member. */
static int load_field(BfmVillageField *field, const MusashiDiscMedia *disc,
                      const uint8_t *scene, size_t size, uint32_t room,
                      uint32_t face_table)
{
    const uint32_t base = 0x80128158u;
    const size_t face = face_table - base;
    if (!scene || face > size || size - face < 0x12d0 + 28) return 0;
    field->guard = (const int8_t *)(scene + face + 0x12bc);
    field->blade = (const int8_t *)(scene + face + 0x12d0);
    field->field = bfm_bedroom_field_load(scene, size, room, 300);
    field->hero = bfm_bedroom_figure_load(disc, scene, size, room, face_table,
                                          face_table + 0xd6c);
    if (!field->field || !field->hero ||
        !bfm_castle_blade_load(&field->fusion, scene, size, face + 0x1110)) {
        bfm_village_field_clear(field);
        return 0;
    }
    return 1;
}

int bfm_village_field_load(BfmVillageField *field, const MusashiDiscMedia *disc,
                           const uint8_t *scene, size_t size, uint32_t room)
{
    return load_field(field, disc, scene, size, room, 0x80187328u);
}

int bfm_village_field_load_lower(BfmVillageField *field, const MusashiDiscMedia *disc,
                                 const uint8_t *scene, size_t size, uint32_t room)
{
    return load_field(field, disc, scene, size, room, 0x8018f338u);
}

int bfm_village_field_prepare(BfmVillageField *field, const BfmVillageCamera *camera,
                              const BfmBedroomRoot *root, const BfmBedroomPose *pose,
                              const uint8_t flags[64], unsigned field_age,
                              uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!bfm_bedroom_field_set_focal(field->field, camera->focal)) return 0;
    bfm_bedroom_field_viewpoint(field->field, camera->eye, camera->target, 1, 1);
    BfmBedroomBody body = {
        .pose = pose->pose, .pose_bytes = 256, .heading = root->yaw, .pitch = pose->pitch,
        .face_word = pose->face_word, .shadow_visible = root->shadow_visible,
        .visible_parts = 0x7fffffff, .blade_tick = field_age,
        .green_blade = (flags[129 / 8] >> (129 % 8)) & 1u
    };
    for (unsigned axis = 0; axis < 3; ++axis) {
        body.position[axis] = root->xyz[axis];
        body.surface_normal[axis] = root->normal[axis];
        body.shadow_position[axis] = root->shadow_xyz[axis];
        body.shadow_normal[axis] = root->shadow_normal[axis];
    }
    if (pose->show_weapon && !(flags[303 / 8] & (1u << (303 % 8)))) {
        body.visible_parts &= ~(1u << 21);
        body.blade = &field->fusion;
    } else {
        body.visible_parts &= ~(1u << 22);
    }
    if (!(flags[128 / 8] & 1u)) {
        body.visible_parts &= ~((1u << 24) | (1u << 25));
    } else if (pose->show_lumina) {
        body.visible_parts &= ~(1u << 24);
        body.lumina_guard = field->guard;
        body.lumina_blade = field->blade;
    } else {
        body.visible_parts &= ~(1u << 25);
    }
    return bfm_bedroom_figure_prepare(field->hero, bfm_bedroom_field_view(field->field),
                                       &body, tiles, pixels);
}
