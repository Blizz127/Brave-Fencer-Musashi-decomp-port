/* SPDX-License-Identifier: MIT */
#include "woods_leno_actor.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { LENO_PARTS = 12, DOG_CLASS = 239, BONE_CLASS = 242 };
static const uint32_t origin = UINT32_C(0x80128158), room = UINT32_C(0x8018ea38);

typedef struct {
    const uint8_t *keys;
    unsigned count;
} LenoClip;

struct BfmWoodsLenoActor {
    const uint8_t *scene, *shadow, *sprite;
    size_t bytes;
    MusashiForestPlayerMesh *mesh;
    LenoClip motion[BFM_LENO_CLIPS];
    BfmBedroomRoot dog, bone;
};

static const uint8_t *mounted(const BfmWoodsLenoActor *art, uint32_t address, size_t size)
{
    if (address < origin || address-origin > art->bytes || size > art->bytes-(address-origin))
        return NULL;
    return art->scene + (address-origin);
}

static int read_homes(BfmWoodsLenoActor *art)
{
    const uint8_t *header = mounted(art, room, 0xbc);
    if (!header) return 0;
    uint32_t first = u32le(header+0xb4), end = u32le(header+0xb8);
    if (end <= first || (end-first)%20) return 0;
    const uint8_t *rows = mounted(art, first, end-first);
    if (!rows) return 0;
    int found_dog = 0, found_bone = 0;
    for (size_t i = 0; i < end-first; i += 20) {
        const uint8_t *row = rows+i;
        unsigned kind = u16le(row+6);
        if (!kind) return found_dog && found_bone;
        if (kind != DOG_CLASS && kind != BONE_CLASS) continue;
        BfmBedroomRoot *root;
        if (kind == DOG_CLASS) {
            if (found_dog++) return 0;
            root = &art->dog;
        } else {
            if (found_bone++) return 0;
            root = &art->bone;
        }
        for (unsigned k = 0; k < 3; ++k) root->xyz[k] = s16le(row+2*k);
        root->yaw = u16le(row+12) & 4095;
        /* The regular placement ray starts at Y-4, sixteen units lower
         * than the actor-shadow ray. Both return the contact surface Y-2. */
        if (root->xyz[1] > INT16_MAX-16) return 0;
        root->xyz[1] += 16;
        if (!bfm_bedroom_travel_air_shadow(art->scene, art->bytes, room, root) ||
            !root->shadow_visible) return 0;
        memcpy(root->xyz, root->shadow_xyz, sizeof root->xyz);
    }
    return 0;
}

static int read_clip(BfmWoodsLenoActor *art, BfmWoodsLenoClip id, uint32_t address)
{
    LenoClip *clip = &art->motion[id];
    clip->keys = mounted(art, address, 8);
    if (!clip->keys) return 0;
    for (unsigned i = 0; i < 128; ++i) {
        const uint8_t *row = mounted(art, address + 8*i, 8);
        if (!row) return 0;
        uint32_t key = u32le(row), duration = u32le(row+4);
        if (!key) {
            if (!i || duration != 0xffff) return 0;
            clip->count = i;
            return 1;
        }
        if ((key >> 24) != 0x81 || (key & 3) || duration != 1 ||
            !mounted(art, key & ~UINT32_C(0x01000000), LENO_PARTS*8)) return 0;
    }
    return 0;
}

BfmWoodsLenoActor *bfm_woods_leno_actor_open(const uint8_t *scene, size_t bytes)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsLenoActor *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    art->scene = scene;
    art->bytes = bytes;
    if (!read_homes(art)) goto fail;
    const uint8_t *profile = mounted(art, UINT32_C(0x8018b7e4), 12);
    if (!profile) goto fail;
    unsigned parts;
    art->mesh = musashi_forest_player_mesh_from_table(scene, bytes, origin, u32le(profile+4), &parts);
    if (!art->mesh || parts != LENO_PARTS) goto fail;

    static const uint32_t clips[BFM_LENO_CLIPS] = {
        [BFM_LENO_REST] = UINT32_C(0x801c01a8),   [BFM_LENO_BARK] = UINT32_C(0x801c0178),
        [BFM_LENO_APPROACH] = UINT32_C(0x801c03f0), [BFM_LENO_CHEW] = UINT32_C(0x801c02a8),
        [BFM_LENO_HELD] = UINT32_C(0x801c00d0),   [BFM_LENO_TOSSED] = UINT32_C(0x801c0028),
        [BFM_LENO_LANDED] = UINT32_C(0x801c0200)
    };
    for (unsigned i = 0; i < BFM_LENO_CLIPS; ++i)
        if (!read_clip(art, (BfmWoodsLenoClip)i, clips[i])) goto fail;
    art->shadow = mounted(art, UINT32_C(0x80189128), 12);
    const uint8_t *bone_clip = mounted(art, UINT32_C(0x8018b91c), 16);
    if (!art->shadow || !bone_clip || !u32le(bone_clip+4) || u32le(bone_clip+8) ||
        u32le(bone_clip+12) != 0xffff) goto fail;
    const uint8_t *s = mounted(art, u32le(bone_clip), 12);
    if (!s || s[0] != 255 || !s[2] || !s[3]) goto fail;
    unsigned x = u16le(s+4), y = u16le(s+6);
    if (x >= 1024 || y >= 512 || 4*(x%64)+s[2] > 255 || y%256+s[3] > 255) goto fail;
    art->sprite = s;
    return art;
fail:
    bfm_woods_leno_actor_free(art);
    return NULL;
}

void bfm_woods_leno_actor_free(BfmWoodsLenoActor *art)
{
    if (!art) return;
    musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

int bfm_woods_leno_actor_home(const BfmWoodsLenoActor *art, int16_t dog[3], int16_t bone[3], int *yaw)
{
    if (!art || !dog || !bone || !yaw) return 0;
    memcpy(dog, art->dog.xyz, sizeof art->dog.xyz);
    memcpy(bone, art->bone.xyz, sizeof art->bone.xyz);
    *yaw = (int)art->dog.yaw;
    return 1;
}

unsigned bfm_woods_leno_actor_duration(const BfmWoodsLenoActor *art, BfmWoodsLenoClip clip)
{
    return art && (unsigned)clip < BFM_LENO_CLIPS ? art->motion[clip].count : 0;
}

static int projected_span(int extent, int reciprocal)
{
    /* The bone's type-3 billboard uses 1.5 times the signed local extent. */
    int scaled = 3*extent/2 - (extent < 0 && extent%2);
    int64_t product = (int64_t)scaled * reciprocal;
    return (int)(product/65536 - (product%65536 < 0));
}

static int add_bone(const BfmWoodsLenoActor *art, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    int p[3];
    if (!bfm_bedroom_project(view, art->bone.xyz, p) || p[2]/4+1 >= 4096) return 1;
    int64_t ratio = ((int64_t)view->focal*65536 + p[2]/2)/p[2];
    int reciprocal = ratio > 131071 ? 131071 : (int)ratio;
    const uint8_t *s = art->sprite;
    int left = p[0]+projected_span(s16le(s+8), reciprocal);
    int top = p[1]+projected_span(s16le(s+10), reciprocal);
    int w = projected_span(s[2], reciprocal), h = projected_span(s[3], reciprocal);
    if (!w || !h || left >= 320 || top >= 240 || left+w <= 0 || top+h <= 0) return 1;
    unsigned tx = u16le(s+4), ty = u16le(s+6), u = 4*(tx%64), v = ty%256;
    unsigned palette_x = s[1] < 224 ? 352 : 256;
    BfmForestDustCard card = {.bucket=(unsigned)(p[2]/4+1),
        .material={.textured=1, .count=4, .page=(uint16_t)(tx/64 | (ty/256)<<4),
                   .clut=(uint16_t)((256u+s[1])*64u+palette_x/16)}};
    /* The encoded extents are the inclusive far UV deltas. */
    for (unsigned i = 0; i < 4; ++i) {
        int right = i&1, bottom = i>>1;
        card.vertices[i] = (BfmForestRasterVertex){left+right*w, top+bottom*h,
            (uint8_t)(u+right*s[2]), (uint8_t)(v+bottom*s[3]), 0x808080};
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}

int bfm_woods_leno_actor_add(const BfmWoodsLenoActor *art, BfmBedroomFigure *figure,
                             const BfmBedroomView *view, const BfmWoodsLenoView *dog)
{
    if (!art || !figure || !view || view->focal <= 0 || view->focal > INT16_MAX ||
        (dog && (unsigned)dog->clip >= BFM_LENO_CLIPS)) return 0;
    if (!add_bone(art, figure, view)) return 0;
    if (!dog) return 1;

    const LenoClip *clip = &art->motion[dog->clip];
    uint32_t key = u32le(clip->keys + 8*(dog->tick%clip->count)) & ~UINT32_C(0x01000000);
    BfmForestPose pose;
    if (!bfm_forest_actor_pose_decode(mounted(art,key,8*LENO_PARTS), 8*LENO_PARTS, LENO_PARTS, &pose))
        return 0;
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw((unsigned)dog->pitch, (unsigned)dog->yaw, dog->xyz, &root);
    BfmBedroomRoot ground = {.yaw=(unsigned)dog->yaw};
    memcpy(ground.xyz, dog->xyz, sizeof ground.xyz);
    if (!bfm_bedroom_travel_air_shadow(art->scene, art->bytes, room, &ground)) return 0;
    BfmForestActor body = {.mesh=art->mesh, .pose=&pose, .root=&root,
                           .visible_parts=dog->visible ? (1u<<LENO_PARTS)-1u : 0};
    return bfm_bedroom_figure_add_actor(figure, view, &body, &ground, art->shadow, 8192);
}
