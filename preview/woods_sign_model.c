/* SPDX-License-Identifier: MIT */
#include "woods_sign_model.h"
#include "forest_guard_contact.h"
#include "forest_raster.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <stdlib.h>

static const uint32_t origin = UINT32_C(0x80128158);
enum { CORNERS = 4, SIDES = 2 };

typedef struct {
    uint16_t indices[CORNERS];
    BfmForestRasterVertex corners[CORNERS];
    BfmForestRasterMaterial material;
} SignSide;

struct BfmWoodsSignModel {
    int16_t position[3], world[CORNERS][3];
    BfmForestTransform root;
    BfmForestGuardBox box;
    SignSide sides[SIDES];
    BfmForestRaster raster;
    int side, depth;
};

static const uint8_t *mounted(const uint8_t *scene, size_t size, uint32_t address, size_t length)
{
    if (!scene || address < origin || address - origin > size || length > size - (address - origin))
        return NULL;
    return scene + (address - origin);
}

static int read_model(BfmWoodsSignModel *sign, const MusashiForestPlayerPart *part,
                      const uint8_t *placement)
{
    if (!part || part->vertex_count != CORNERS || part->face_count != SIDES) return 0;
    for (unsigned k = 0; k < 3; ++k) sign->position[k] = s16le(placement + 2*k);
    bfm_forest_actor_root(u16le(placement + 12),sign->position,&sign->root);
    for (unsigned i = 0; i < CORNERS; ++i) {
        const MusashiForestPlayerVector *v = &part->vertices[i];
        for (unsigned k = 0; k < 3; ++k) {
            const int16_t *axis = sign->root.rotation[k];
            int64_t dot = (int64_t)axis[0]*v->x + (int64_t)axis[1]*v->y + (int64_t)axis[2]*v->z;
            int value = sign->position[k] + (int)(dot/4096 - (dot%4096 < 0));
            if (value < INT16_MIN || value > INT16_MAX) return 0;
            sign->world[i][k] = (int16_t)value;
        }
    }
    /* Both sides are textured, unlit quads on the same four vertices. The
     * reverse side has its own vertex and UV order. */
    for (unsigned i = 0; i < SIDES; ++i) {
        const MusashiForestPlayerFace *face = part->faces + i;
        if (face->mode != 0x2d || face->corners != CORNERS || (face->tpage & 0x180)) return 0;
        SignSide *side = &sign->sides[i];
        side->material = (BfmForestRasterMaterial){
            .count = CORNERS, .textured = 1, .page = face->tpage, .clut = face->clut
        };
        for (unsigned k = 0; k < CORNERS; ++k) {
            side->indices[k] = face->vertex[k];
            side->corners[k] = (BfmForestRasterVertex){.u=face->u[k], .v=face->v[k], .rgb=0x808080};
        }
    }
    return 1;
}

BfmWoodsSignModel *bfm_woods_sign_model_load(const uint8_t *scene, size_t bytes, uint32_t room)
{
    const uint8_t *descriptor = mounted(scene,bytes,room,0xbc);
    if (!descriptor || room != UINT32_C(0x8018ea38)) return NULL;
    uint32_t first = u32le(descriptor+0xb4), end = u32le(descriptor+0xb8);
    if (end <= first || (end-first)%20) return NULL;
    const uint8_t *placement = mounted(scene,bytes,first,end-first);
    if (!placement) return NULL;
    size_t offset = 0;
    while (offset < end-first && u16le(placement+offset+6) != 0x341) {
        if (!u16le(placement+offset+6)) return NULL;
        offset += 20;
    }
    if (offset == end-first) return NULL;
    const uint8_t *resource = mounted(scene,bytes,UINT32_C(0x8018bdb8),12);
    if (!resource) return NULL;
    const uint8_t *shape = mounted(scene,bytes,u32le(resource+8),16);
    if (!shape || u32le(shape)) return NULL;
    BfmForestGuardBox box;
    for (unsigned k = 0; k < 3; ++k) {
        box.low[k] = s16le(shape+4+4*k);
        box.high[k] = s16le(shape+6+4*k);
        if (box.low[k] > box.high[k]) return NULL;
    }
    unsigned count;
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_from_table(
        scene,bytes,origin,u32le(resource+4),&count);
    if (!mesh) return NULL;
    BfmWoodsSignModel *sign = calloc(1,sizeof *sign);
    if (sign && (count != 1 || !read_model(sign,musashi_forest_player_mesh_part(mesh,0),placement+offset))) {
        free(sign);
        sign = NULL;
    }
    musashi_forest_player_mesh_free(mesh);
    if (sign) {
        sign->box = box;
        sign->depth = INT_MIN;
    }
    return sign;
}

void bfm_woods_sign_model_free(BfmWoodsSignModel *sign)
{
    free(sign);
}

const int16_t *bfm_woods_sign_model_position(const BfmWoodsSignModel *sign)
{
    return sign ? sign->position : NULL;
}

int bfm_woods_sign_model_slide(const BfmWoodsSignModel *sign,
                               const int16_t previous[3], int16_t endpoint[3])
{
    if (!sign || !previous || !endpoint || previous[1] < INT16_MIN+16 || endpoint[1] < INT16_MIN+16)
        return 0;
    const int16_t start[3] = {previous[0], previous[1]-16, previous[2]};
    const int16_t end[3] = {endpoint[0], endpoint[1]-16, endpoint[2]};
    int16_t hit[3];
    if (!bfm_forest_body_slide(&sign->box,&sign->root,start,end,hit) || hit[1] > INT16_MAX-16)
        return 0;
    endpoint[0] = hit[0];
    endpoint[1] = hit[1]+16;
    endpoint[2] = hit[2];
    return 1;
}

int bfm_woods_sign_model_prepare(BfmWoodsSignModel *sign, const BfmBedroomView *view,
                                 const uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!sign || !view || !tiles || !pixels) return 0;
    sign->raster = (BfmForestRaster){.tiles=tiles,.pixels=(uint32_t (*)[320])pixels,.top=10,.bottom=229};
    sign->depth = INT_MIN;
    int projected[CORNERS][3], sum = 0;
    for (unsigned k = 0; k < CORNERS; ++k) {
        int *p = projected[k];
        if (!bfm_bedroom_project(view,sign->world[k],p) || p[2] <= view->focal/2) return 1;
        if (p[0] <= 160-1024 || p[0] >= 160+1023 || p[1] <= 120-1024 || p[1] >= 120+1023)
            return 1;
        sum += p[2];
    }
    for (unsigned i = 0; i < SIDES; ++i) {
        SignSide *side = &sign->sides[i];
        BfmForestRasterVertex *c = side->corners;
        for (unsigned k = 0; k < CORNERS; ++k) {
            const int *p = projected[side->indices[k]];
            c[k].x = p[0];
            c[k].y = p[1];
        }
        if ((c[1].x-c[0].x)*(c[2].y-c[0].y) > (c[1].y-c[0].y)*(c[2].x-c[0].x)) {
            sign->depth = sum / (4*CORNERS);
            sign->side = (int)i;
            break;
        }
    }
    return 1;
}

int bfm_woods_sign_model_next_depth(const BfmWoodsSignModel *sign)
{
    return sign ? sign->depth : INT_MIN;
}

void bfm_woods_sign_model_draw_to(BfmWoodsSignModel *sign, int depth)
{
    if (sign && sign->depth != INT_MIN && sign->depth >= depth) {
        const SignSide *side = &sign->sides[sign->side];
        bfm_field_raster_face(&sign->raster,&side->material,side->corners);
        sign->depth = INT_MIN;
    }
}
