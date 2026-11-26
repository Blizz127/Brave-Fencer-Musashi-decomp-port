/* SPDX-License-Identifier: MIT */
#include "southern_woods.h"
#include "village_field.h"
#include "village_sky.h"
#include "scene_tiles.h"
#include <stdlib.h>

struct BfmSouthernWoods {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    BfmVillageMotion *motion;
    BfmVillageField field;
    BfmVillageCamera camera;
    uint16_t (*tiles)[1024];
    unsigned age;
};

static void follow(BfmSouthernWoods *woods, int entering)
{
    const BfmBedroomRoot *root = &bfm_village_motion_player(woods->motion)->root;
    BfmVillageCamera *cam = &woods->camera;
    const int offset[3] = {295, -809, 259};
    for (unsigned axis = 0; axis < 3; ++axis) {
        int target = root->xyz[axis] - (axis == 1 ? 128 : 0);
        int difference = target - cam->target[axis];
        int step = difference / 4;
        if (!step && difference) step = difference < 0 ? -1 : 1;
        cam->target[axis] = entering ? target : cam->target[axis] + step;
        cam->eye[axis] = cam->target[axis] + offset[axis];
    }
}

void bfm_southern_woods_free(BfmSouthernWoods *woods)
{
    if (!woods) return;
    bfm_village_motion_free(woods->motion);
    bfm_village_field_clear(&woods->field);
    free(woods);
}

BfmSouthernWoods *bfm_southern_woods_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
    const uint8_t *door, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    BfmBedroomEntry next = *entry;
    if (!bfm_castle_room_entry(disc,door,&next) || next.scene_member != 29) return NULL;
    BfmSouthernWoods *woods = calloc(1,sizeof *woods);
    if (!woods) return NULL;
    woods->bytes = bfm_read_archive_field_scene(disc,"SC03.CD",next.scene_member,woods->scene);
    woods->motion = bfm_village_motion_woods(woods->scene,woods->bytes,main,main_bytes);
    if (!woods->motion || !bfm_village_field_load_woods(&woods->field,disc,woods->scene,woods->bytes) ||
        !bfm_scene_read_tiles(disc,"MAIN.CD",10,tiles) ||
        !bfm_scene_read_tiles(disc,"SC03.CD",next.scene_member,tiles)) {
        bfm_southern_woods_free(woods);
        return NULL;
    }
    woods->tiles = tiles;
    woods->camera.focal = 400;
    follow(woods,1);
    *entry = next;
    return woods;
}

const BfmVillagePlayer *bfm_southern_woods_player(const BfmSouthernWoods *woods)
{
    return bfm_village_motion_player(woods->motion);
}

int bfm_southern_woods_step(BfmSouthernWoods *woods, int x, int y)
{
    int yaw = bfm_village_motion_view_yaw(woods->camera.eye,woods->camera.target);
    if (!bfm_village_motion_step(woods->motion,x,y,yaw)) return 0;
    follow(woods,0);
    ++woods->age;
    return bfm_village_motion_face(woods->motion,woods->tiles);
}

int bfm_southern_woods_draw(BfmSouthernWoods *woods, const uint8_t flags[64],
                            uint32_t pixels[240][320])
{
    const BfmVillagePlayer *p = bfm_southern_woods_player(woods);
    const size_t room = 0x8018ea38u - 0x80128158u;
    if (!bfm_village_sky_draw(woods->scene+room,woods->bytes-room,pixels) ||
        !bfm_village_field_prepare(&woods->field,&woods->camera,&p->root,&p->pose,flags,
                                   woods->age,woods->tiles,&pixels[0][0])) return 0;
    bfm_bedroom_field_draw(woods->field.field,woods->tiles,&pixels[0][0],
                            bfm_bedroom_figure_draw_to,woods->field.hero);
    return 1;
}
