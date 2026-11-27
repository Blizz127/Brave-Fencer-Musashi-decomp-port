/* SPDX-License-Identifier: MIT */
#include "woods_king_art.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { VERTICES = 133, MAX_KEYS = 7 };
static const uint32_t origin = UINT32_C(0x80128158);

typedef struct {
    const uint8_t *vertices, *deltas;
    unsigned first, count, duration;
    int reverse;
} KingKey;

typedef struct {
    KingKey keys[MAX_KEYS];
    unsigned count, duration;
    int loop;
} KingClip;

struct BfmWoodsKingArt {
    MusashiForestPlayerMesh *mesh;
    MusashiForestPlayerPart part;
    MusashiForestPlayerVector vertices[VERTICES];
    KingClip clips[3];
    const uint8_t *shadow;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (address < origin || address - origin > bytes || length > bytes - (address - origin)) return NULL;
    return scene + (address - origin);
}

static int load_key(KingKey *key, const uint8_t *scene, size_t bytes, uint32_t descriptor)
{
    const uint8_t *record = mounted(scene, bytes, descriptor, 12);
    if (!record || u32le(record + 8)) return 0;  /* These clips keep the original surface normals. */
    uint32_t shape = u32le(record), delta = u32le(record + 4);
    const uint8_t *model = mounted(scene, bytes, shape, 40);
    const uint8_t *span = mounted(scene, bytes, delta, 16);
    if (!model || !span || u32le(model) != 0x41 || u32le(model + 4) ||
        u32le(model + 8) != 1 || u32le(model + 16) != VERTICES ||
        u32le(span) != 1 || u32le(span + 4)) return 0;
    uint32_t vertex_offset = u32le(model + 12);
    if (delta < shape || delta - shape < 12 + VERTICES * 8 ||
        vertex_offset > delta - shape - 12 - VERTICES * 8) return 0;
    key->vertices = model + 12 + vertex_offset;
    key->first = u32le(span + 8);
    key->count = u32le(span + 12);
    if (key->first > VERTICES || !key->count || key->count > VERTICES - key->first ||
        !mounted(scene, bytes, delta, 16 + key->count * 8)) return 0;
    key->deltas = span + 16;
    return 1;
}

static int load_clip(KingClip *clip, const uint8_t *scene, size_t bytes, uint32_t list)
{
    for (unsigned i = 0; i <= MAX_KEYS; ++i) {
        const uint8_t *row = mounted(scene, bytes, list + 8*i, 8);
        if (!row) return 0;
        uint32_t descriptor = u32le(row);
        if (!descriptor || descriptor == UINT32_MAX) {
            clip->loop = !descriptor;
            return clip->count && !u32le(row + 4);
        }
        if (i == MAX_KEYS) return 0;
        KingKey *key = &clip->keys[i];
        int step = s16le(row + 6);
        key->duration = u16le(row + 4);
        if (!key->duration || (step != 1 && step != -1) || !load_key(key, scene, bytes, descriptor)) return 0;
        key->reverse = step < 0;
        clip->duration += key->duration;
        ++clip->count;
    }
    return 0;
}

BfmWoodsKingArt *bfm_woods_king_art_open(const uint8_t *scene, size_t bytes)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsKingArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    const uint8_t *resource = mounted(scene, bytes, UINT32_C(0x8018b720), 12);
    unsigned parts;
    if (!resource) goto fail;
    art->mesh = musashi_forest_player_mesh_from_table(scene, bytes, origin, u32le(resource + 4), &parts);
    const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(art->mesh, 0);
    if (!part || parts != 1 || part->vertex_count != VERTICES) goto fail;
    art->part = *part;
    art->part.vertices = art->vertices;
    art->shadow = mounted(scene, bytes, UINT32_C(0x80189128), 12);
    if (!art->shadow ||
        !load_clip(&art->clips[BFM_KING_IDLE], scene, bytes, UINT32_C(0x8018b690)) ||
        !load_clip(&art->clips[BFM_KING_BITE], scene, bytes, UINT32_C(0x8018b6e8)) ||
        !load_clip(&art->clips[BFM_KING_HURT], scene, bytes, UINT32_C(0x8018b6a8))) goto fail;
    return art;
fail:
    bfm_woods_king_art_free(art);
    return NULL;
}

void bfm_woods_king_art_free(BfmWoodsKingArt *art)
{
    if (!art) return;
    musashi_forest_player_mesh_free(art->mesh);
    free(art);
}

static MusashiForestPlayerVector vector(const uint8_t *bytes)
{
    return (MusashiForestPlayerVector){s16le(bytes), s16le(bytes + 2), s16le(bytes + 4)};
}

static int16_t displaced(int source, int delta, int weight)
{
    int product = delta * weight;
    return (int16_t)(source + product / 4096 - (product % 4096 < 0));
}

static void apply_key(MusashiForestPlayerVector *vertices, const KingKey *key, unsigned time)
{
    int weight = (int)(4096 * time / key->duration);
    if (key->reverse) weight = 4096 - weight;
    for (unsigned i = 0; i < key->count; ++i) {
        unsigned index = key->first + i;
        MusashiForestPlayerVector src = vector(key->vertices + 8*index), d = vector(key->deltas + 8*i);
        vertices[index] = (MusashiForestPlayerVector){
            displaced(src.x, d.x, weight), displaced(src.y, d.y, weight), displaced(src.z, d.z, weight)
        };
    }
}

int bfm_woods_king_art_add(BfmWoodsKingArt *art, BfmBedroomFigure *figure, const BfmBedroomView *view,
                           const BfmBedroomRoot *root, BfmWoodsKingPose pose, unsigned elapsed,
                           unsigned horizontal_q12, unsigned vertical_q12)
{
    if (!art || (unsigned)pose >= 3) return 0;
    const KingClip *clip = &art->clips[pose];
    if (clip->loop) elapsed %= clip->duration;
    else if (elapsed >= clip->duration) elapsed = clip->duration - 1;
    /* A key replaces only its own contiguous span. Start from the entry
     * shape, retaining the final submission of each preceding span. The
     * bite loop starts with a full-span key; idle repeats a single span. */
    for (unsigned i = 0; i < VERTICES; ++i)
        art->vertices[i] = vector(clip->keys[0].vertices + 8*i);
    for (unsigned i = 0; i < clip->count; ++i) {
        const KingKey *key = &clip->keys[i];
        if (elapsed < key->duration) {
            apply_key(art->vertices, key, elapsed);
            break;
        }
        apply_key(art->vertices, key, key->duration - 1);
        elapsed -= key->duration;
    }
    return bfm_bedroom_figure_add_morph(figure, view, &art->part, root, horizontal_q12,
                                        vertical_q12, art->shadow, 8192);
}
