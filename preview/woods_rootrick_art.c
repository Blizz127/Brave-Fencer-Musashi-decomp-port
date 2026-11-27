/* SPDX-License-Identifier: MIT */
#include "woods_rootrick_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { PARTS = 13, MEMBER = 12, SECTOR = 2048, PAYLOAD = 0x8be4, CLIP_CAPACITY = 24 };
static const uint32_t field_base = UINT32_C(0x80128158), cast_base = UINT32_C(0x801eba80);
static const uint32_t first_clip = UINT32_C(0x801f3ea4);

typedef struct {
    uint32_t start, end, next;
    unsigned ticks;
    int loop;
} RootrickClip;

struct BfmWoodsRootrickArt {
    const uint8_t *scene;
    size_t scene_bytes;
    MusashiForestPlayerMesh *mesh;
    RootrickClip clips[CLIP_CAPACITY];
    unsigned count;
    uint8_t payload[PAYLOAD];
};

static int read_performer(const MusashiDiscMedia *disc, uint8_t payload[PAYLOAD])
{
    uint8_t directory[SECTOR], header[16];
    size_t bytes;
    if (!musashi_disc_media_read_root_range(disc, "SC02.CD", 0, directory, sizeof directory, &bytes))
        return 0;
    unsigned members = u32le(directory);
    if (members <= MEMBER || members > (SECTOR - 8) / 8) return 0;
    uint32_t sector = u32le(directory + 8 + 8*MEMBER);
    uint32_t size = u32le(directory + 12 + 8*MEMBER);
    if (!sector || sector > bytes / SECTOR) return 0;
    size_t offset = (size_t)sector * SECTOR;
    return size <= bytes - offset && size >= SECTOR + PAYLOAD &&
           musashi_disc_media_read_root_range(disc, "SC02.CD", offset, header, sizeof header, &bytes) &&
           u32le(header) == 0x434150 && u32le(header+4) == 0x101 && !u32le(header+8) &&
           u32le(header+12) == SECTOR + PAYLOAD &&
           musashi_disc_media_read_root_range(disc, "SC02.CD", offset+SECTOR, payload, PAYLOAD, &bytes);
}

static const RootrickClip *lookup(const BfmWoodsRootrickArt *art, uint32_t address)
{
    for (unsigned i = 0; i < art->count; ++i)
        if (art->clips[i].start == address) return &art->clips[i];
    return NULL;
}

static int load_clips(BfmWoodsRootrickArt *art, uint32_t first_pose)
{
    uint32_t address = first_clip;
    while (address < cast_base + PAYLOAD) {
        if (art->count == CLIP_CAPACITY) return 0;
        RootrickClip *clip = &art->clips[art->count++];
        clip->start = address;
        for (;;) {
            if (address > cast_base + PAYLOAD - 8) return 0;
            const uint8_t *row = art->payload + (address-cast_base);
            uint32_t pose = u32le(row), duration = u32le(row+4);
            address += 8;
            if (!pose || duration == 0xfffc) {
                if (!clip->ticks || (!pose && duration != 0xffff && duration != 0xfffe)) return 0;
                clip->end = address - 8;
                clip->next = pose;
                clip->loop = duration == 0xffff;
                break;
            }
            unsigned stride = (pose & UINT32_C(0x01000000)) ? 8 : 12;
            pose &= ~UINT32_C(0x01000000);
            if (pose < first_pose || pose > first_clip - PARTS*stride || (pose & 3) ||
                !duration || duration > 0x7fff) return 0;
            clip->ticks += duration;
        }
    }
    /* The two continuation commands in this member enter ordinary loops.
     * Validate both destinations before making the actor drawable. */
    for (unsigned i = 0; i < art->count; ++i) {
        if (!art->clips[i].next) continue;
        const RootrickClip *tail = lookup(art, art->clips[i].next);
        if (!tail || tail->next) return 0;
    }
    return 1;
}

BfmWoodsRootrickArt *bfm_woods_rootrick_art_open(const MusashiDiscMedia *disc,
                                                 const uint8_t *scene, size_t bytes)
{
    size_t capacity = cast_base - field_base;
    if (!disc || !scene || bytes > capacity || capacity - bytes >= 16) return NULL;
    BfmWoodsRootrickArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    art->scene = scene;
    art->scene_bytes = bytes;
    const uint8_t *profile = scene + (UINT32_C(0x801950a4) - field_base);
    uint32_t table = u32le(profile+4);
    unsigned parts;
    if (!read_performer(disc, art->payload) || u32le(art->payload) != 103) goto fail;
    art->mesh = musashi_forest_player_mesh_from_table(art->payload, PAYLOAD, cast_base, table, &parts);
    if (!art->mesh || parts != PARTS || !load_clips(art, table + 4*(PARTS+1))) goto fail;
    return art;
fail:
    bfm_woods_rootrick_art_free(art);
    return NULL;
}

void bfm_woods_rootrick_art_free(BfmWoodsRootrickArt *art)
{
    if (!art) return;
    musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

static int sample(const BfmWoodsRootrickArt *art, uint32_t address, unsigned elapsed,
                  BfmForestPose *pose)
{
    const RootrickClip *clip = lookup(art, address);
    if (!clip) return 0;
    if (clip->next && elapsed >= clip->ticks) {
        elapsed -= clip->ticks;
        clip = lookup(art, clip->next);
    }
    if (clip->loop) elapsed %= clip->ticks;
    else if (elapsed >= clip->ticks) elapsed = clip->ticks - 1;
    for (uint32_t key = clip->start; key < clip->end; key += 8) {
        const uint8_t *row = art->payload + (key-cast_base);
        unsigned duration = u32le(row+4);
        if (elapsed < duration) {
            uint32_t encoded = u32le(row);
            uint32_t frame = encoded & ~UINT32_C(0x01000000);
            const uint8_t *data = art->payload + (frame-cast_base);
            return frame == encoded ? bfm_forest_actor_pose_decode_full(data, PARTS*12, PARTS, pose) :
                                      bfm_forest_actor_pose_decode(data, PARTS*8, PARTS, pose);
        }
        elapsed -= duration;
    }
    return 0;
}

int bfm_woods_rootrick_art_visible(const BfmWoodsRootrickArt *art, BfmBedroomFigure *figure,
                               const BfmBedroomView *view, const int16_t xyz[3],
                               unsigned yaw, int pitch, uint32_t clip_address, unsigned elapsed,
                               int show_model, int show_shadow)
{
    BfmForestPose pose;
    if (!art || !figure || !view || !xyz || !sample(art, clip_address, elapsed, &pose)) return 0;
    BfmForestTransform transform;
    bfm_forest_actor_pitch_yaw((unsigned)pitch, yaw, xyz, &transform);
    BfmForestActor body = {.mesh=art->mesh, .root=&transform, .pose=&pose,
                           .visible_parts=show_model ? (1u << PARTS)-1 : 0};
    BfmBedroomRoot ground = {.xyz={xyz[0], xyz[1], xyz[2]}, .yaw=yaw};
    if (show_shadow && !bfm_bedroom_travel_air_shadow(art->scene, art->scene_bytes, UINT32_C(0x801961e8), &ground))
        return 0;
    const uint8_t *shadow = art->scene + (UINT32_C(0x80192648)-field_base);
    return bfm_bedroom_figure_add_actor(figure, view, &body, &ground, shadow, 8192);
}

int bfm_woods_rootrick_art_add(const BfmWoodsRootrickArt *art, BfmBedroomFigure *figure,
                               const BfmBedroomView *view, const int16_t xyz[3],
                               unsigned yaw, int pitch, uint32_t clip_address, unsigned elapsed)
{
    return bfm_woods_rootrick_art_visible(art, figure, view, xyz, yaw, pitch, clip_address, elapsed, 1, 1);
}
