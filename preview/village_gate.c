/* SPDX-License-Identifier: MIT */
#include "village_gate.h"
#include "forest_pose.h"
#include "forest_raster.h"
#include <limits.h>
#include <stdlib.h>

enum { LEAVES = 2, QUADS = 4, CORNERS = 4 };

typedef struct {
    int16_t local[CORNERS][3];
    BfmForestRasterMaterial material;
    BfmForestRasterVertex corners[CORNERS];
    int depth;
} GateQuad;

typedef struct {
    int16_t hinge[3];
    GateQuad quads[QUADS];
} GateLeaf;

struct BfmVillageGate {
    GateLeaf leaves[LEAVES];
    BfmForestRaster raster;
    GateQuad *order[LEAVES * QUADS];
    unsigned visible, next;
    int left, right, top, bottom;
};

static unsigned half(const uint8_t *p)
{
    return p[0] | ((unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p)
{
    return half(p) | ((uint32_t)half(p + 2) << 16);
}

static int signed_half(const uint8_t *p)
{
    int n = (int)half(p);
    return n < 32768 ? n : n - 65536;
}

static const uint8_t *at(const uint8_t *scene, size_t size, uint32_t address, size_t length)
{
    const uint32_t base = 0x80128158u;
    if (address < base || address - base > size || length > size - (address - base)) return NULL;
    return scene + (address - base);
}

static int q12(int64_t x)
{
    return (int)(x < 0 ? -((-x + 4095) / 4096) : x / 4096);
}

/* The two single-object TMDs have unlit textured quads. Each quad stores
 * four UVs, a color word and four vertex indices without normal indices. */
static int load_leaf(GateLeaf *leaf, const uint8_t *scene, size_t size, unsigned variant, int opened)
{
    uint32_t table = variant ? 0x801c8b2cu : 0x801c8a3cu;
    const uint8_t *link = at(scene,size,table,8);
    const uint8_t *hinge = at(scene,size,0x801c8b34u + 8 * variant,6);
    if (!hinge || !link || word(link + 4)) return 0;
    uint32_t model = word(link);
    const uint8_t *tmd = at(scene,size,model,40);
    if (!tmd || model > table || table-model < 40 ||
        word(tmd) != 0x41 || word(tmd + 4) || word(tmd + 8) != 1 || word(tmd + 36)) return 0;
    size_t limit = table - model - 12;
    const uint8_t *object = tmd + 12;
    size_t verts = word(object), count = word(object + 4), faces = word(object + 16);
    if (faces < 28 || faces > verts || verts - faces != 32 * QUADS ||
        verts > limit || count > (limit - verts) / 8 || !count || word(object + 20) != QUADS) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) leaf->hinge[axis] = (int16_t)signed_half(hinge + 2*axis);
    int heading = opened ? (variant ? 1536 : -1536) : 0;
    BfmForestTransform transform;
    bfm_forest_actor_root((unsigned)heading,leaf->hinge,&transform);
    for (unsigned i = 0; i < QUADS; ++i) {
        const uint8_t *face = object + faces + 32*i;
        if (face[0] != 9 || face[1] != 7 || face[2] != 1 || face[3] != 0x2d) return 0;
        GateQuad *quad = &leaf->quads[i];
        unsigned page = half(face + 10), clut = half(face + 6);
        if ((page & 0x180) || (clut >> 6) >= 512) return 0;
        quad->material = (BfmForestRasterMaterial){.page=page,.clut=clut,.count=CORNERS,.textured=1};
        for (unsigned k = 0; k < CORNERS; ++k) {
            unsigned index = half(face + 24 + 2*k);
            if (index >= count) return 0;
            const uint8_t *v = object + verts + 8*index;
            int x = signed_half(v), z = signed_half(v + 4);
            int rx = q12((int64_t)transform.rotation[0][0]*x + (int64_t)transform.rotation[0][2]*z);
            int rz = q12((int64_t)transform.rotation[2][0]*x + (int64_t)transform.rotation[2][2]*z);
            if (rx < INT16_MIN || rx > INT16_MAX || rz < INT16_MIN || rz > INT16_MAX) return 0;
            quad->local[k][0] = (int16_t)rx;
            quad->local[k][1] = (int16_t)signed_half(v + 2);
            quad->local[k][2] = (int16_t)rz;
            quad->corners[k] = (BfmForestRasterVertex){
                .u=face[4 + 4*k], .v=face[5 + 4*k], .rgb=0x808080
            };
        }
    }
    return 1;
}

BfmVillageGate *bfm_village_gate_load(const uint8_t *scene, size_t size, uint32_t room,
                                      unsigned progress, const uint8_t flags[64])
{
    if (!scene || !flags || size > BFM_FIELD_SCENE_CAPACITY) return NULL;
    const uint8_t *descriptor = at(scene,size,room,0xb8);
    if (!descriptor) return NULL;
    uint32_t address = word(descriptor + 0xb4);
    unsigned found = 0;
    BfmVillageGate *gate = calloc(1,sizeof(*gate));
    if (!gate) return NULL;
    int opened = progress >= 230 || (flags[246/8] & (1u << (246%8)));
    for (const uint8_t *row; (row = at(scene,size,address,20)); address += 20) {
        unsigned type = half(row + 6);
        if (!type) {
            if (found == 3) return gate;
            break;
        }
        if (type != 498) continue;
        unsigned variant = half(row + 8);
        if (variant >= LEAVES || (found & (1u << variant)) ||
            !load_leaf(&gate->leaves[variant],scene,size,variant,opened)) break;
        found |= 1u << variant;
    }
    free(gate);
    return NULL;
}

void bfm_village_gate_free(BfmVillageGate *gate)
{
    free(gate);
}

int bfm_village_gate_prepare(BfmVillageGate *gate, const BfmBedroomView *view,
                             const uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!gate || !view || !tiles || !pixels) return 0;
    gate->raster = (BfmForestRaster){.tiles=tiles,.pixels=(uint32_t (*)[320])pixels,.top=10,.bottom=229};
    gate->next = gate->visible = 0;
    gate->left = 320;
    gate->right = -1;
    gate->top = 230;
    gate->bottom = 9;
    for (unsigned leaf = 0; leaf < LEAVES; ++leaf) {
        BfmBedroomView local = *view;
        for (unsigned i = 0; i < 3; ++i) {
            int64_t offset = 0;
            for (unsigned k = 0; k < 3; ++k)
                offset += (int64_t)view->axes[i][k] * gate->leaves[leaf].hinge[k];
            local.translation[i] += q12(offset);
        }
        for (unsigned i = 0; i < QUADS; ++i) {
            GateQuad *quad = &gate->leaves[leaf].quads[i];
            int sum = 0;
            unsigned k = 0;
            for (; k < CORNERS; ++k) {
                int p[3];
                if (!bfm_bedroom_project(&local,quad->local[k],p) || p[2] <= local.focal/2) break;
                quad->corners[k].x = p[0];
                quad->corners[k].y = p[1];
                sum += p[2];
            }
            if (k != CORNERS) continue;
            const BfmForestRasterVertex *p = quad->corners;
            if ((p[1].x-p[0].x)*(p[2].y-p[0].y) <= (p[1].y-p[0].y)*(p[2].x-p[0].x)) continue;
            for (unsigned j = 0; j < CORNERS; ++j) {
                if (p[j].x < gate->left) gate->left = p[j].x < 0 ? 0 : p[j].x;
                if (p[j].x > gate->right) gate->right = p[j].x > 319 ? 319 : p[j].x;
                if (p[j].y < gate->top) gate->top = p[j].y < 10 ? 10 : p[j].y;
                if (p[j].y > gate->bottom) gate->bottom = p[j].y > 229 ? 229 : p[j].y;
            }
            quad->depth = sum/16;
            unsigned pos = gate->visible++;
            while (pos && gate->order[pos-1]->depth <= quad->depth) {
                gate->order[pos] = gate->order[pos-1];
                --pos;
            }
            gate->order[pos] = quad;
        }
    }
    return 1;
}

void bfm_village_gate_draw_to(BfmVillageGate *gate, int depth)
{
    if (!gate || !gate->raster.pixels) return;
    while (gate->next < gate->visible && gate->order[gate->next]->depth >= depth) {
        const GateQuad *quad = gate->order[gate->next++];
        bfm_forest_raster_face(&gate->raster,&quad->material,quad->corners);
    }
    /* Map the shared raster's five-bit colors to the field scanout levels. */
    if (depth == INT_MIN) {
        for (int y = gate->top; y <= gate->bottom; ++y) {
            for (int x = gate->left; x <= gate->right; ++x) {
                uint32_t pixel = gate->raster.pixels[y][x], rgb = pixel & 0xff000000u;
                for (unsigned bit = 0; bit < 24; bit += 8)
                    rgb |= (((pixel >> (bit + 3)) & 31) * 255 / 31) << bit;
                gate->raster.pixels[y][x] = rgb;
            }
        }
    }
}
