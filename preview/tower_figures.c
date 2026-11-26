/* SPDX-License-Identifier: MIT */
#include "tower_figures.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { CENTER_PARTS = 4, COLUMN_PARTS = 7, COLUMN_SHAPE = 4, OPEN_PLATE, USED_PLATE, SHAPES };
static const uint32_t scene_origin = UINT32_C(0x80128158);

typedef struct {
    int16_t roots[4][3], pivot_x, pivot_y;
    uint8_t width, height;
    BfmForestRasterMaterial frame[2];
    uint8_t u[2], v[2];
} BfmTowerGlow;

struct BfmTowerFigures {
    BfmTowerGlow glow;
    BfmTowerPressColors press_colors;
    MusashiForestPlayerMesh *mesh[SHAPES];
    BfmForestPose pose[SHAPES];
    BfmForestTransform root[BFM_TOWER_FIGURE_ACTORS];
    int16_t center[3], placement[BFM_TOWER_POSTS][3];
    BfmForestTransform column_root[BFM_TOWER_POSTS];
    BfmForestPose damaged_pose[4], column_pose[BFM_TOWER_POSTS];
    uint8_t loose_joint[4];
};

static int glow_load(BfmTowerGlow *glow, const uint8_t *scene, size_t size)
{
    enum { anchors=0x622bc, sprite=0x62304, frames=0x62320 };
    if (!glow || !scene || size < frames + 16) return 0;
    const uint8_t *desc = scene + sprite;
    BfmTowerGlow g = {.width=desc[2],.height=desc[3],
                      .pivot_x=s16le(desc+8),.pivot_y=s16le(desc+10)};
    if (desc[0] != 255 || !g.width || !g.height) return 0;
    for (unsigned n = 0; n < 4; ++n)
        for (unsigned k = 0; k < 3; ++k)
            g.roots[n][k] = (int16_t)s16le(scene + anchors + 16*n + 2*k);
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *cell = scene + frames + 8*i;
        unsigned x = u16le(cell), y = u16le(cell+2), palette = u16le(cell+4);
        if (x >= 1024 || y >= 512 || palette > 255 ||
            4*(x%64)+g.width > 256 || y%256+g.height > 256) return 0;
        g.u[i] = (uint8_t)(4*(x%64));
        g.v[i] = (uint8_t)(y%256);
        g.frame[i] = (BfmForestRasterMaterial){
            .count=4, .page=(uint16_t)(x/64 | (y/256)<<4 | 32),
            .clut=(uint16_t)((256+palette)*64 + (palette<224 ? 352 : 256)/16),
            .textured=1, .translucent=1
        };
    }
    *glow = g;
    return 1;
}

static int64_t floor_ratio(int64_t x, int divisor)
{
    return x/divisor - (x%divisor < 0);
}

/* The sprite projects its pivot and extent independently of its center. The
 * halo uses the same camera-facing plane and is centered on the same origin. */
static int bulb(const BfmTowerGlow *g, const int16_t root[3],
                const BfmForestCamera *camera, unsigned phase, unsigned parity,
                BfmForestDustCard cards[7])
{
    int16_t center[3];
    for (unsigned row=0; row<3; ++row) {
        int64_t dot=0;
        for (unsigned i=0; i<3; ++i) dot += (int64_t)camera->rotation[row][i]*root[i];
        int64_t p=camera->translation[row]+floor_ratio(dot,4096);
        if (p < INT16_MIN || p > INT16_MAX) return 0;
        center[row]=(int16_t)p;
    }
    int depth=center[2];
    if (depth <= 0 || camera->projection >= 2*depth) return 0;
    BfmForestCamera plane={.rotation={{4096,0,0},{0,4096,0},{0,0,4096}},
                           .projection=camera->projection};
    int16_t v[4][3]={{center[0],center[1],center[2]}, {g->pivot_x,g->pivot_y,center[2]},
                     {g->width,g->height,center[2]}, {0,0,center[2]}};
    BfmForestDustCard box={0};
    if (!bfm_forest_project_effect_quad(&plane,v,&box) || ++box.bucket >= 4096) return 0;
    BfmForestRasterVertex mid=box.vertices[0], zero=box.vertices[3];
    int x=mid.x + box.vertices[1].x-zero.x, y=mid.y + box.vertices[1].y-zero.y;
    int width=box.vertices[2].x-zero.x, height=box.vertices[2].y-zero.y;
    if (width <= 0 || height <= 0) return 0;
    box.material=g->frame[phase];
    for (unsigned i=0; i<4; ++i) {
        unsigned right=i%2, bottom=i/2;
        box.vertices[i]=(BfmForestRasterVertex){
            x+(int)right*width,y+(int)bottom*height,
            (uint8_t)(g->u[phase]+right*(g->width-1)),
            (uint8_t)(g->v[phase]+bottom*(g->height-1)),0x808080
        };
    }
    cards[0]=box;

    BfmForestRasterVertex rim[12]={{0}};
    const int16_t origin[3]={0};
    for (unsigned i=0; i<12; ++i) {
        unsigned acute = i<=3 ? i : i<=6 ? 6-i : i<=9 ? i-6 : 12-i;
        BfmForestTransform turn;
        bfm_forest_actor_root(341*acute,origin,&turn);
        int dx=(int)floor_ratio(80*turn.rotation[0][2],4096);
        int dy=(int)floor_ratio(80*turn.rotation[0][0],4096);
        if (i>6) dx=-dx;
        if (i>3 && i<9) dy=-dy;
        v[i%4][0]=(int16_t)dx;
        v[i%4][1]=(int16_t)dy;
        v[i%4][2]=center[2];
        if (i%4!=3) continue;
        BfmForestDustCard group={0};
        if (!bfm_forest_project_effect_quad(&plane,v,&group)) return 0;
        for (unsigned j=0; j<4; ++j) {
            rim[i-3+j].x=mid.x + group.vertices[j].x-zero.x;
            rim[i-3+j].y=mid.y + group.vertices[j].y-zero.y;
        }
    }
    mid.rgb=parity ? 0x80a040 : 0x709030;
    for (unsigned wedge=0; wedge<6; ++wedge) {
        unsigned a=2*wedge;
        cards[wedge+1]=(BfmForestDustCard){
            .material={.count=4,.page=32,.translucent=1},.bucket=box.bucket,
            .vertices={rim[a],rim[(a+1)%12],mid,rim[(a+2)%12]}
        };
    }
    return 1;
}

static const uint8_t *mounted(const uint8_t *scene, size_t size,
                              uint32_t address, size_t count)
{
    if (!scene || address < scene_origin) return NULL;
    size_t offset = address - scene_origin;
    return offset <= size && count <= size - offset ? scene + offset : NULL;
}

/* These scene poses store signed XYZ and angles as six full halfwords.
 * Keep their translations in world units after decoding the rotations. */
static int room_pose(const uint8_t *data, unsigned parts, BfmForestPose *pose)
{
    uint8_t packed[8 * COLUMN_PARTS] = {0};
    for (unsigned i = 0; i < parts; ++i) {
        const uint8_t *joint = data + 12*i;
        uint8_t *out = packed + 8*i;
        uint16_t y = u16le(joint + 8), z = u16le(joint + 10);
        out[0] = (uint8_t)(((z >> 4) & 0xf0) | ((y >> 8) & 15));
        out[1] = (uint8_t)y;
        out[2] = (uint8_t)z;
        out[6] = joint[6];
        out[7] = joint[7];
    }
    if (!bfm_forest_actor_pose_decode(packed, sizeof packed, parts, pose)) return 0;
    for (unsigned i = 0; i < parts; ++i)
        for (unsigned axis = 0; axis < 3; ++axis)
            pose->local[i].translation[axis] = s16le(data + 12*i + 2*axis);
    return 1;
}

static int load_shape(BfmTowerFigures *figures, unsigned id, const uint8_t *scene, size_t size,
                      uint32_t table, uint32_t clip, unsigned parts)
{
    const uint8_t *key = mounted(scene, size, clip, 8);
    if (!key || u32le(key + 4) != 1) return 0;
    const uint8_t *frame = mounted(scene, size, u32le(key), 12*parts);
    if (!frame || !room_pose(frame, parts, &figures->pose[id])) return 0;
    unsigned count = 0;
    figures->mesh[id] = musashi_forest_player_mesh_from_table(scene, size, scene_origin,
                                                               table, &count);
    return figures->mesh[id] && count == parts;
}

BfmTowerFigures *bfm_tower_figures_open(const uint8_t *scene, size_t size)
{
    /* The room figures using these mounts have the same visible local
     * transforms across their entrance clips. Their actor roots are separate. */
    static const struct { uint32_t shape, clip; } mount[COLUMN_SHAPE + 1] = {
        {0x801a37a4, 0x801899c4}, {0x801ae2d8, 0x8018a02c},
        {0x801ae850, 0x8018a044}, {0x801acc1c, 0x80189d04},
        {0x80188970, 0x80186f44}
    };
    const uint8_t *records = mounted(scene, size, UINT32_C(0x8018a708), 5*20);
    if (!records || u16le(records + 6) != 9) return NULL;
    BfmTowerFigures *figures = calloc(1, sizeof *figures);
    if (!figures) return NULL;
    if (!glow_load(&figures->glow, scene, size) ||
        !bfm_tower_press_colors_load(&figures->press_colors, scene, size)) goto failed;
    for (unsigned i = 0; i <= COLUMN_SHAPE; ++i)
        if (!load_shape(figures, i, scene, size, mount[i].shape, mount[i].clip,
                        i == COLUMN_SHAPE ? COLUMN_PARTS : 1)) goto failed;

    const uint8_t *keys = mounted(scene, size, UINT32_C(0x80186fb0), 5*4);
    const uint8_t *pieces = mounted(scene, size, UINT32_C(0x80186f8c), 4);
    if (!keys || !pieces) goto failed;
    for (unsigned hp = 0; hp < 4; ++hp) {
        const uint8_t *key = mounted(scene, size, u32le(keys + hp*4), 8);
        const uint8_t *frame = key ? mounted(scene, size, u32le(key), COLUMN_PARTS*12) : NULL;
        if (!frame || pieces[hp] >= COLUMN_PARTS ||
            !room_pose(frame, COLUMN_PARTS, &figures->damaged_pose[hp])) goto failed;
        figures->loose_joint[hp] = pieces[hp];
    }
    const uint32_t plates[] = { UINT32_C(0x801891ac), UINT32_C(0x80189450) };
    for (unsigned i = 0; i < 2; ++i) {
        unsigned shape = OPEN_PLATE + i, count = 0;
        figures->mesh[shape] = musashi_forest_player_mesh_from_table(scene, size, scene_origin,
                                                                      plates[i], &count);
        if (!figures->mesh[shape] || count != 1) goto failed;
        bfm_forest_actor_root(0, (const int16_t[3]){0}, &figures->pose[shape].local[0]);
    }
    for (unsigned k = 0; k < 3; ++k)
        figures->center[k] = s16le(records + 2*k);
    for (unsigned i = 0; i < CENTER_PARTS; ++i)
        bfm_forest_actor_root(0, figures->center, &figures->root[i]);
    for (unsigned i = 0; i < 4; ++i) {
        const uint8_t *placement = records + 20*(i+1);
        if (u16le(placement + 6) != 20 || u16le(placement + 8) != i)
            goto failed;
        int16_t position[3];
        for (unsigned k = 0; k < 3; ++k) {
            position[k] = s16le(placement + 2*k);
            figures->placement[i][k] = position[k];
        }
        BfmForestTransform *root = &figures->root[CENTER_PARTS + i];
        bfm_forest_actor_root(u16le(placement + 12), position, root);
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned col = 0; col < 3; ++col) {
                int v = root->rotation[row][col] * 7;
                root->rotation[row][col] = (int16_t)(v / 4 - (v % 4 < 0));
            }
        figures->column_root[i] = *root;
    }
    return figures;
failed:
    bfm_tower_figures_free(figures);
    return NULL;
}

void bfm_tower_figures_free(BfmTowerFigures *figures)
{
    if (!figures) return;
    for (unsigned i = 0; i < SHAPES; ++i) musashi_forest_player_mesh_free(figures->mesh[i]);
    free(figures);
}

size_t bfm_tower_figures_actors(BfmTowerFigures *figures, unsigned age,
                                const BfmTowerPylonFrame posts[BFM_TOWER_POSTS],
                                BfmForestActor *out, size_t capacity)
{
    if (!figures || !posts || !out || capacity < BFM_TOWER_FIGURE_ACTORS) return 0;
    /* The body installs the statue and its base; both inherit its 448-unit
     * lift on their next update. The base uses the same integral model Y. */
    figures->root[1].translation[1] = figures->center[1] - (age ? 448 : 0);
    figures->root[2].translation[1] = figures->center[1] - (age ? 448 : 1);
    for (unsigned i = 0; i < CENTER_PARTS; ++i) {
        out[i] = (BfmForestActor){figures->mesh[i], &figures->pose[i], &figures->root[i], 1u, NULL};
    }
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i) {
        const BfmTowerPylonFrame *post = &posts[i];
        unsigned actor = CENTER_PARTS + i, shape = COLUMN_SHAPE;
        BfmForestTransform *root = &figures->root[actor];
        const BfmForestPose *pose;
        if (post->phase >= BFM_TOWER_EXPOSED) {
            shape = post->phase == BFM_TOWER_CREDITED ? USED_PLATE : OPEN_PLATE;
            bfm_forest_actor_root(post->plate_yaw, figures->placement[i], root);
            pose = &figures->pose[shape];
        } else {
            *root = figures->column_root[i];
            pose = post->remaining == 4 ? &figures->pose[shape] : &figures->damaged_pose[post->remaining];
            if (post->phase == BFM_TOWER_FRACTURE) {
                unsigned joint = figures->loose_joint[post->remaining];
                int shift = post->shake_x;
                BfmForestPose *moving = &figures->column_pose[i];
                *moving = *pose;
                for (unsigned axis = 0; axis < 3; ++axis) {
                    root->translation[axis] += (int32_t)floor_ratio(shift*root->rotation[axis][0], 4096);
                    moving->local[joint].translation[axis] = figures->pose[shape].local[joint].translation[axis]
                        + post->fragment[axis] - (axis == 0 ? shift : 0);
                }
                pose = moving;
            }
        }
        out[actor] = (BfmForestActor){figures->mesh[shape], pose, root,
            shape == COLUMN_SHAPE ? (1u << COLUMN_PARTS)-1 : 1u, NULL};
    }
    return BFM_TOWER_FIGURE_ACTORS;
}

size_t bfm_tower_figures_cards(const BfmTowerFigures *figures, unsigned age,
                               const BfmTowerPylonFrame posts[BFM_TOWER_POSTS], unsigned field_parity,
                               const BfmForestCamera *camera, BfmForestDustCard *out, size_t capacity)
{
    if (!figures || !age || !posts || !camera || !camera->projection || !out ||
        capacity < BFM_TOWER_FIGURE_CARDS) return 0;
    size_t count = 0;
    unsigned lit_nodes = bfm_tower_lit_nodes(posts);
    for (unsigned i = 0; i < 4; ++i)
        if ((lit_nodes & (1u << i)) && bulb(&figures->glow, figures->glow.roots[i],
                camera, age & 1, field_parity & 1, out + count)) count += 7;
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i)
        if (posts[i].phase == BFM_TOWER_PRESSING)
            count += bfm_tower_press_cards(figures->placement[i], posts[i].ticks, camera,
                                          &figures->press_colors, out + count);
    return count;
}
