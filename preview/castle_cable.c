/* SPDX-License-Identifier: MIT */
#include "castle_cable.h"
#include "cable_sparks.h"
#include "bedroom_field.h"
#include "bedroom_figure.h"
#include "bedroom_motion.h"
#include "bedroom_travel.h"
#include "forest_raster.h"
#include "scene_tiles.h"
#include <stdlib.h>
#include <string.h>

enum { MAX_SECTIONS = 64 };
static const uint32_t base = 0x80128158u;
typedef struct {
    BfmForestRasterVertex vertex[4];
    int depth;
    unsigned order, spark;
} CableSection;
struct BfmCastleCable {
    uint8_t scene[BFM_FIELD_SCENE_CAPACITY];
    size_t bytes;
    uint32_t room;
    uint16_t (*tiles)[1024];
    BfmBedroomField *field;
    BfmBedroomFigure *figure;
    BfmBedroomMotion *motion;
    BfmBedroomPose pose;
    BfmForestBlade fusion;
    const int8_t *lumina_guard, *lumina_blade;
    int target[3], yaw, pitch;
    int16_t origin[3], position[3], ends[2][3];
    int velocity, distance, heading;
    unsigned age, movement, count, next;
    uint32_t uv[4];
    BfmCableSparks *sparks;
    BfmCableSparkFace spark_faces[8];
    CableSection sections[MAX_SECTIONS + 8];
    BfmForestRaster surface;
};
static unsigned half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}
static uint32_t word(const uint8_t *p)
{
    return half(p) | (uint32_t)half(p+2) << 16;
}
static const uint8_t *record(const BfmCastleCable *c, uint32_t address, size_t size)
{
    if (address < base || size > c->bytes || address-base > c->bytes-size) return NULL;
    return c->scene+(address-base);
}
void bfm_castle_cable_free(BfmCastleCable *c)
{
    if (!c) return;
    bfm_cable_sparks_free(c->sparks);
    bfm_bedroom_field_free(c->field);
    bfm_bedroom_figure_free(c->figure);
    bfm_bedroom_motion_free(c->motion);
    free(c);
}

BfmCastleCable *bfm_castle_cable_open(const MusashiDiscMedia *disc, BfmBedroomEntry *entry,
                                     const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024])
{
    BfmBedroomEntry next = *entry;
    if (word(entry->state.destination)!=0x3004 ||
        !bfm_castle_room_entry(disc,entry->state.destination,&next)) return NULL;
    BfmCastleCable *c = calloc(1,sizeof *c);
    if (!c) return NULL;
    c->bytes = bfm_read_field_scene(disc,next.scene_member,c->scene);
    const uint8_t *root = record(c,0x8017e5acu,4);
    const uint8_t *room = root ? record(c,word(root),0xcc) : NULL;
    const uint8_t *entrances = room ? record(c,word(room+0xc8),4) : NULL;
    const uint8_t *door = entrances ? record(c,word(entrances),36) : NULL;
    const uint8_t *actors = room ? record(c,word(room+0xb4),40) : NULL;
    const uint8_t *aim = record(c,0x80182234u,6);
    const uint8_t *ends = record(c,0x8019a8b0u,16);
    const uint8_t *uv = record(c,0x80182244u,16);
    if (!door || !actors || half(actors+6)!=405 || half(actors+26)!=645 || !aim || !ends || !uv)
        goto fail;
    c->room = word(root);
    c->heading = half(door+22);
    if (c->heading!=2048) goto fail;
    for (unsigned k=0;k<3;++k) {
        c->origin[k] = c->position[k] = (int16_t)half(door+16+2*k);
        c->target[k] = (int16_t)half(aim+2*k);
        for (unsigned i=0;i<2;++i) c->ends[i][k] = (int16_t)half(ends+8*i+2*k);
    }
    for (unsigned i=0;i<4;++i) c->uv[i] = word(uv+4*i);
    if (c->ends[1][2]<=c->ends[0][2] || c->ends[1][2]-c->ends[0][2] > 64*(MAX_SECTIONS-2))
        goto fail;
    c->target[1] -= 25;
    c->pitch = 398;
    c->yaw = 3552;
    c->tiles = tiles;
    c->lumina_guard=(const int8_t *)record(c,0x801816e0u,18);
    c->lumina_blade=(const int8_t *)record(c,0x801816f4u,28);
    if (!c->lumina_guard || !c->lumina_blade || !bfm_castle_blade_load(&c->fusion,
                  c->scene,c->bytes,0x80181534u-base)) goto fail;
    c->field = bfm_bedroom_field_load(c->scene,c->bytes,word(root),300);
    c->figure = bfm_bedroom_figure_load(disc,c->scene,c->bytes,word(root),0x80180424u,0x80181190u);
    const uint8_t *orientations = record(c,0x801815dcu,16);
    if (orientations) c->sparks = bfm_cable_sparks_open(orientations,16);
    c->motion = bfm_bedroom_motion_open(c->scene,c->bytes,main,main_bytes,word(root));
    if (!c->field || !c->figure || !c->motion || !c->sparks ||
        !bfm_bedroom_field_panorama(c->field,c->scene,c->bytes,word(root)) ||
        !bfm_scene_read_tiles(disc,"MAIN.CD",10,tiles) ||
        !bfm_scene_read_tiles(disc,"SC01.CD",next.scene_member,tiles) ||
        !bfm_bedroom_motion_step(c->motion,BFM_BEDROOM_CABLE,&c->pose)) goto fail;
    *entry = next;
    return c;
fail:
    bfm_castle_cable_free(c);
    return NULL;
}

static void span(BfmCastleCable *c, const int16_t a[3], const int16_t b[3])
{
    if (b[2]<=a[2]) return;
    int prev_y=a[1], prev_z=a[2];
    while (prev_z<b[2] && c->count<MAX_SECTIONS) {
        int z=prev_z+64;
        /* The terminal vertex clamps to the anchor after evaluating its slope. */
        int y=a[1]+(b[1]-a[1])*(z-a[2])/(b[2]-a[2]);
        if (z>b[2]) z=b[2];
        CableSection *s=&c->sections[c->count];
        unsigned k=0;
        for (;k<4;++k) {
            int16_t xyz[3]={a[0],(int16_t)(((k&1)?y:prev_y)+(k<2?-4:4)),(int16_t)((k&1)?z:prev_z)};
            int point[3];
            if (!bfm_bedroom_project(bfm_bedroom_field_view(c->field),xyz,point) ||
                point[2] <= bfm_bedroom_field_view(c->field)->focal/2 ||
                point[0]<-1024 || point[0]>1023 || point[1]<-1024 || point[1]>1023) break;
            if (!k) s->depth=point[2]/4<4095 ? point[2]/4 : 4095;
            s->vertex[k]=(BfmForestRasterVertex){point[0],point[1],(uint8_t)c->uv[k],
                                                (uint8_t)(c->uv[k]>>16),0x808080};
        }
        if (k == 4) {
            s->spark = 0;
            s->order = c->count++;
        }
        prev_z=z;
        prev_y=y;
    }
}

static int far_first(const void *a, const void *b)
{
    const CableSection *x=a,*y=b;
    if (x->depth!=y->depth) return y->depth-x->depth;
    return (int)y->order-(int)x->order;
}
static void draw_to(void *context, int depth)
{
    BfmCastleCable *c=context;
    const BfmForestRasterMaterial material={.page=0x1a,.clut=0x5416,.count=4,.textured=1};
    while (c->next<c->count && c->sections[c->next].depth>=depth) {
        CableSection *s=&c->sections[c->next++];
        bfm_bedroom_figure_draw_to(c->figure,s->depth+1);
        if (s->spark) bfm_cable_sparks_draw(&c->surface,&c->spark_faces[s->spark-1]);
        else bfm_forest_raster_face(&c->surface,&material,s->vertex);
    }
    bfm_bedroom_figure_draw_to(c->figure,depth);
}

int bfm_castle_cable_step(BfmCastleCable *c, const uint8_t flags[64], BfmGameplayRun *run,
                         uint8_t destination[36], uint32_t pixels[240][320])
{
    ++c->age;
    if (c->pitch>0) c->pitch=c->pitch<5 ? 0 : c->pitch-5;
    if (c->yaw<4064) c->yaw=c->yaw>4058 ? 4064 : c->yaw+6;
    if (!bfm_bedroom_field_cable_view(c->field,c->target,c->yaw,c->pitch)) return 0;
    if (c->age>2) {
        ++c->movement;
        if (c->velocity<62) ++c->velocity;
        c->distance+=c->velocity;
        c->position[1]+=2;
        c->position[2]=(int16_t)(c->origin[2]+c->distance/2);
        bfm_cable_sparks_step(c->sparks,c->movement,c->position,run);
        if (c->movement==65) {
            const uint8_t *door=record(c,0x8018202cu,36);
            if (!door || word(door)!=0x00051011u) return 0;
            memcpy(destination,door,36);
        }
    }
    if ((c->age>1 && !bfm_bedroom_motion_step(c->motion,BFM_BEDROOM_NONE,&c->pose)) ||
        !bfm_bedroom_motion_face(c->motion,c->tiles)) return 0;
    BfmBedroomBody body={.pose=c->pose.pose,.pose_bytes=256,.heading=c->heading,
                         .surface_normal={0,-4096,0},.face_word=c->pose.face_word,
                         .visible_parts=0x7fffffffu};
    body.blade_tick = c->age;
    body.green_blade = (flags[129/8] >> (129%8)) & 1u;
    if (c->pose.show_weapon && !(flags[303/8] & (1u << (303%8)))) {
        body.visible_parts &= ~(1u << 21);
        body.blade = &c->fusion;
    } else {
        body.visible_parts &= ~(1u << 22);
    }
    if (!(flags[128/8] & 1u)) {
        body.visible_parts &= ~((1u << 24) | (1u << 25));
    } else if (c->pose.show_lumina) {
        body.visible_parts &= ~(1u << 24);
        body.lumina_guard = c->lumina_guard;
        body.lumina_blade = c->lumina_blade;
    } else {
        body.visible_parts &= ~(1u << 25);
    }
    BfmBedroomRoot ground = {0};
    memcpy(ground.xyz,c->position,sizeof ground.xyz);
    if (!bfm_bedroom_travel_air_shadow(c->scene,c->bytes,c->room,&ground)) return 0;
    body.shadow_visible = ground.shadow_visible;
    for (unsigned k=0;k<3;++k) {
        body.position[k] = c->position[k];
        body.shadow_position[k] = ground.shadow_xyz[k];
        body.shadow_normal[k] = ground.shadow_normal[k];
    }
    for (unsigned y=0;y<240;++y) for (unsigned x=0;x<320;++x) pixels[y][x]=0xff000000u;
    const BfmBedroomView *view=bfm_bedroom_field_view(c->field);
    if (!bfm_bedroom_figure_prepare(c->figure,view,&body,c->tiles,&pixels[0][0])) return 0;
    c->next=c->count=0;
    int16_t contact[3]={c->position[0],(int16_t)(c->position[1]-64),c->position[2]};
    if (contact[2]>=c->ends[0][2]) {
        span(c,c->ends[0],contact);
        span(c,contact,c->ends[1]);
    } else span(c,c->ends[0],c->ends[1]);
    unsigned sparks = bfm_cable_sparks_project(c->sparks,view,c->spark_faces);
    for (unsigned i = 0; i < sparks; ++i) {
        CableSection *s = &c->sections[c->count];
        s->depth = c->spark_faces[i].depth;
        s->spark = i + 1;
        s->order = c->count++;
    }
    qsort(c->sections,c->count,sizeof c->sections[0],far_first);
    c->surface=(BfmForestRaster){.tiles=c->tiles,.pixels=pixels,.top=10,.bottom=229};
    bfm_bedroom_field_draw(c->field,c->tiles,&pixels[0][0],draw_to,c);
    return 1;
}
