/* SPDX-License-Identifier: MIT */
#include "village_cable.h"
#include "cable_sparks.h"
#include <limits.h>
#include <stdlib.h>

enum { CORNERS = 4, SECTION_WIDTH = 64, MAX_SPARK_FACES = 8 };
typedef struct {
    BfmForestRasterVertex corner[CORNERS];
    int depth;
    unsigned order, spark;
} CableFace;

struct BfmVillageCable {
    int end[2][3];
    uint8_t uv[CORNERS][2];
    BfmForestRaster surface;
    BfmForestRasterMaterial material;
    BfmCableSparks *sparks;
    BfmCableSparkFace spark_face[MAX_SPARK_FACES];
    unsigned count, next;
    CableFace faces[];
};

static unsigned half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static int signed_half(const uint8_t *p)
{
    int v = (int)half(p);
    return v < 32768 ? v : v - 65536;
}

static uint32_t word(const uint8_t *p)
{
    return half(p) | (uint32_t)half(p + 2) << 16;
}

static const uint8_t *at(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    const uint32_t base = 0x80128158u;
    if (address < base || address-base > bytes || length > bytes-(address-base)) return NULL;
    return scene + (address-base);
}

static int placed_wire(const uint8_t *scene, size_t bytes, const uint8_t *room)
{
    uint32_t placements = word(room + 0xb4);
    for (const uint8_t *row; (row = at(scene,bytes,placements,20)); placements += 20) {
        unsigned kind = half(row + 6);
        if (!kind) break;
        if (kind == 0x19c) return 1;
    }
    return 0;
}

BfmVillageCable *bfm_village_cable_load(const uint8_t *scene, size_t bytes, uint32_t room)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    const uint8_t *desc = at(scene,bytes,room,0xcc);
    const uint8_t *anchors = at(scene,bytes,0x801ca324u,16);
    const uint8_t *uv = at(scene,bytes,0x8018931cu,16);
    const uint8_t *rays = at(scene,bytes,0x801884e0u,16);
    if (!desc || !anchors || !uv || !rays || !placed_wire(scene,bytes,desc)) return NULL;
    const uint8_t *entries = at(scene,bytes,word(desc+0xc8),24);
    const uint8_t *arrival = entries ? at(scene,bytes,word(entries+20),24) : NULL;
    int width = signed_half(anchors+8) - signed_half(anchors);
    if (!arrival || width <= 0) return NULL;
    /* Splitting at the rider adds at most one partial section. */
    size_t capacity = (unsigned)(width+SECTION_WIDTH-1)/SECTION_WIDTH + 1 + MAX_SPARK_FACES;
    BfmVillageCable *c = calloc(1,sizeof *c + capacity * sizeof(CableFace));
    if (!c) return NULL;
    for (unsigned i=0; i<2; ++i)
        for (unsigned k=0; k<3; ++k) c->end[i][k] = signed_half(anchors+8*i+2*k);
    for (unsigned i=0; i<CORNERS; ++i) {
        c->uv[i][0] = uv[4*i];
        c->uv[i][1] = uv[4*i+2];
    }
    unsigned page = ((half(uv)>>8)&15u) | ((half(uv+2)>>4)&16u);
    c->material = (BfmForestRasterMaterial){.count=CORNERS, .page=(uint16_t)page,
                                             .clut=0x5416, .textured=1};
    c->sparks = bfm_cable_sparks_open_yaw(rays,16,half(arrival+22));
    if (!c->sparks) {
        free(c);
        return NULL;
    }
    return c;
}

void bfm_village_cable_free(BfmVillageCable *c)
{
    if (c) bfm_cable_sparks_free(c->sparks);
    free(c);
}

void bfm_village_cable_step(BfmVillageCable *c, unsigned tick, int emitting,
                            const int16_t hero_xyz[3], BfmGameplayRun *run)
{
    if (c) bfm_cable_sparks_advance(c->sparks,tick,emitting && tick % 4 == 1,hero_xyz,run);
}

static void append_section(BfmVillageCable *c, const BfmBedroomView *view, const int a[3], const int b[3])
{
    CableFace *face = &c->faces[c->count];
    for (unsigned i=0; i<CORNERS; ++i) {
        const int *p = (i&1) ? b : a;
        int y = p[1] + (i<2 ? -4 : 4);
        if (y<INT16_MIN || y>INT16_MAX || p[2]<INT16_MIN || p[2]>INT16_MAX) return;
        int16_t world[3] = {(int16_t)p[0],(int16_t)y,(int16_t)p[2]};
        int screen[3];
        if (!bfm_bedroom_project(view,world,screen) || screen[2]<=view->focal/2 ||
            screen[0]<-1024 || screen[0]>1023 || screen[1]<-1024 || screen[1]>1023) return;
        if (!i) face->depth = screen[2]/4 < 4095 ? screen[2]/4 : 4095;
        face->corner[i] = (BfmForestRasterVertex){screen[0],screen[1],c->uv[i][0],c->uv[i][1],0x808080};
    }
    face->spark = 0;
    face->order = c->count++;
}

static void project_span(BfmVillageCable *c, const BfmBedroomView *view, const int a[3], const int b[3])
{
    int width = b[0]-a[0];
    int prev[3] = {a[0],a[1],a[2]};
    for (int dx=SECTION_WIDTH; prev[0]<b[0]; dx+=SECTION_WIDTH) {
        /* Y and Z use full section width even when the final X stops early. */
        int next[3] = {dx<width ? a[0]+dx : b[0],0,0};
        for (unsigned k=1; k<3; ++k)
            next[k] = a[k]+(int)((int64_t)(b[k]-a[k])*dx/width);
        append_section(c,view,prev,next);
        for (unsigned k=0; k<3; ++k) prev[k]=next[k];
    }
}

static int far_first(const void *lhs, const void *rhs)
{
    const CableFace *a=lhs, *b=rhs;
    if (a->depth!=b->depth) return b->depth-a->depth;
    return (int)b->order-(int)a->order;
}

int bfm_village_cable_prepare(BfmVillageCable *c, const BfmBedroomView *view,
                              const int16_t hero_xyz[3], int riding,
                              const uint16_t tiles[512][1024], uint32_t *pixels)
{
    if (!c || !view || !tiles || !pixels || (riding && !hero_xyz) || view->focal<=0) return 0;
    c->surface = (BfmForestRaster){.tiles=tiles,.pixels=(uint32_t (*)[320])pixels,.top=10,.bottom=229};
    c->next=c->count=0;
    if (riding && hero_xyz[0]>c->end[0][0] && hero_xyz[0]<c->end[1][0]) {
        int rider[3]={hero_xyz[0],hero_xyz[1]-64,hero_xyz[2]};
        project_span(c,view,c->end[0],rider);
        project_span(c,view,rider,c->end[1]);
    } else {
        project_span(c,view,c->end[0],c->end[1]);
    }
    unsigned sparks = bfm_cable_sparks_project(c->sparks,view,c->spark_face);
    for (unsigned i=0; i<sparks; ++i) {
        CableFace *face=&c->faces[c->count];
        face->depth=c->spark_face[i].depth;
        face->spark=i+1;
        face->order=c->count++;
    }
    qsort(c->faces,c->count,sizeof *c->faces,far_first);
    return 1;
}

int bfm_village_cable_next_depth(const BfmVillageCable *c)
{
    return c && c->next<c->count ? c->faces[c->next].depth : INT_MIN;
}

/* The shared raster expands five-bit channels by repeating their high bits.
 * The village scanout uses the full 0..255 scale for the same channels. */
static void scanout(const BfmForestRaster *surface, const BfmForestRasterVertex *p, unsigned count)
{
    int left=320, top=230, right=-1, bottom=9;
    for (unsigned i=0; i<count; ++i) {
        if (p[i].x<left) left=p[i].x<0 ? 0 : p[i].x;
        if (p[i].x>right) right=p[i].x>319 ? 319 : p[i].x;
        if (p[i].y<top) top=p[i].y<10 ? 10 : p[i].y;
        if (p[i].y>bottom) bottom=p[i].y>229 ? 229 : p[i].y;
    }
    for (int y=top; y<=bottom; ++y) {
        for (int x=left; x<=right; ++x) {
            uint32_t pixel=surface->pixels[y][x], rgb=pixel & 0xffffffu;
            uint32_t high=rgb & 0xf8f8f8u;
            if ((high | ((high>>5) & 0x070707u)) != rgb) continue;
            uint32_t result = pixel & 0xff000000u;
            for (unsigned shift=0; shift<24; shift+=8)
                result |= (((high>>(shift+3)) & 31u)*255/31) << shift;
            surface->pixels[y][x]=result;
        }
    }
}

void bfm_village_cable_draw_to(BfmVillageCable *c, int depth)
{
    if (!c) return;
    while (c->next<c->count && c->faces[c->next].depth>=depth) {
        const CableFace *face=&c->faces[c->next++];
        if (face->spark) {
            const BfmCableSparkFace *spark = &c->spark_face[face->spark-1];
            bfm_cable_sparks_draw(&c->surface,spark);
            scanout(&c->surface,spark->vertices,spark->count);
        } else {
            bfm_forest_raster_face(&c->surface,&c->material,face->corner);
            scanout(&c->surface,face->corner,CORNERS);
        }
    }
}
