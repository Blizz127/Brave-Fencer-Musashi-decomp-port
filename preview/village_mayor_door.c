/* SPDX-License-Identifier: MIT */
#include "village_mayor_door.h"
#include "bakery_door.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"
#include "forest_raster.h"
#include <limits.h>
#include <stdlib.h>

enum { DOOR_VERTICES = 8, DOOR_FACES = 4, FIELD_BASE = 0x80128158u };

typedef struct {
    BfmForestRasterMaterial material;
    BfmForestRasterVertex corners[4];
    uint8_t index[4];
    int depth;
} DoorFace;

struct BfmVillageMayorDoor {
    int16_t hinge[3], vertex[DOOR_VERTICES][3];
    unsigned base_heading, vertices, faces_used;
    DoorFace faces[DOOR_FACES];
    unsigned order[DOOR_FACES], visible, next;
    BfmForestRaster raster;
};

static const uint8_t *at(const uint8_t *scene, size_t bytes, uint32_t address, size_t need)
{
    if (address < FIELD_BASE || address-FIELD_BASE > bytes || need > bytes-(address-FIELD_BASE))
        return NULL;
    return scene+(address-FIELD_BASE);
}

static const uint8_t *placement(const uint8_t *scene, size_t bytes, uint32_t room, unsigned building)
{
    const uint8_t *desc = at(scene,bytes,room,0xbc);
    if (!desc) return NULL;
    uint32_t address = u32le(desc+0xb4), end = u32le(desc+0xb8);
    if (end < address || !at(scene,bytes,address,end-address)) return NULL;
    for (; end-address >= 20; address+=20) {
        const uint8_t *row = at(scene,bytes,address,20);
        unsigned kind = u16le(row+6);
        if (!kind) break;
        if (kind == 97 && u16le(row+8) == building) return row;
    }
    return NULL;
}

static BfmVillageMayorDoor *load_leaf(const uint8_t *scene, size_t bytes, uint32_t room,
                                      unsigned building, int closed)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    const uint8_t *row = placement(scene,bytes,room,building);
    if (!row) return NULL;
    const uint8_t *bank = at(scene,bytes,0x801c0b68u + 2 * building,2);
    if (!bank || s16le(bank) != 0) return NULL; /* Rectangular village door. */
    const uint8_t *entry = at(scene,bytes,closed ? 0x801c0b2cu : 0x801c0b28u,4);
    if (!entry) return NULL;
    const uint8_t *models = at(scene,bytes,u32le(entry),8);
    if (!models || u32le(models+4)) return NULL;
    uint32_t model = u32le(models);
    const uint8_t *tmd = at(scene,bytes,model,40);
    if (!tmd || u32le(tmd)!=0x41 || u32le(tmd+4) || u32le(tmd+8)!=1) return NULL;
    const uint8_t *object = tmd+12;
    size_t body = bytes-(size_t)(object-scene);
    uint32_t verts=u32le(object), prims=u32le(object+16);
    unsigned vertices = closed ? 4 : DOOR_VERTICES, faces = closed ? 1 : DOOR_FACES;
    if (u32le(object+4)!=vertices || u32le(object+20)!=faces ||
        verts>body || vertices*8>body-verts || prims>body || faces*32>body-prims)
        return NULL;
    BfmVillageMayorDoor *door = calloc(1,sizeof(*door));
    if (!door) return NULL;
    for (unsigned k=0;k<3;k++) door->hinge[k]=s16le(row+2*k);
    door->base_heading=u16le(row+12);
    door->vertices=vertices;
    door->faces_used=faces;
    for (unsigned i=0;i<door->vertices;i++)
        for (unsigned k=0;k<3;k++)
            door->vertex[i][k]=s16le(object+verts+8*i+2*k);
    for (unsigned i=0;i<door->faces_used;i++) {
        const uint8_t *p = object+prims+32*i;
        if (p[0]!=9 || p[1]!=7 || p[2]!=1 || p[3]!=0x2d) goto invalid;
        DoorFace *f=&door->faces[i];
        f->material=(BfmForestRasterMaterial){.page=u16le(p+10),.clut=u16le(p+6),.count=4,.textured=1};
        for (unsigned k=0;k<4;k++) {
            unsigned index=u16le(p+24+2*k);
            if (index>=door->vertices) goto invalid;
            f->index[k]=(uint8_t)index;
            f->corners[k]=(BfmForestRasterVertex){.u=p[4+4*k],.v=p[5+4*k],.rgb=u32le(p+20)&0xffffff};
        }
    }
    return door;
invalid:
    free(door);
    return NULL;
}

BfmVillageMayorDoor *bfm_village_mayor_door_load(const uint8_t *scene, size_t bytes, uint32_t room)
{
    return load_leaf(scene,bytes,room,5,0);
}

void bfm_village_mayor_door_free(BfmVillageMayorDoor *door)
{
    free(door);
}

int bfm_village_mayor_door_on_porch(const BfmVillageMayorDoor *door, const int16_t xyz[3])
{
    if (!door || !xyz) return 0;
    int x = xyz[0] - door->hinge[0];
    int z = xyz[2] - door->hinge[2];
    return x >= -192 && x < 64 && z >= -96 && z < 0;
}

int bfm_village_mayor_door_angle(unsigned elapsed, int departing, unsigned release_age)
{
    /* Each update increases the leaf's angular velocity by two units.
     * The open stop is three eighths of a revolution from the jamb. */
    unsigned age = departing ? release_age : elapsed;
    unsigned delay = departing ? 24 : 97;
    unsigned step = age > delay ? age - delay : 0;
    unsigned distance = step < 39 ? step * (step + 1) : 1536;
    return departing ? (int)distance - 1536 : -(int)distance;
}

static int q12(int64_t value)
{
    return (int)(value/4096 - (value%4096<0));
}

void bfm_village_mayor_door_prepare(BfmVillageMayorDoor *door, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels, int angle)
{
    if (!door) return;
    door->visible=door->next=0;
    if (!view || !tiles || !pixels) return;
    door->raster=(BfmForestRaster){tiles,(uint32_t (*)[320])pixels,10,229};
    BfmForestTransform root;
    bfm_forest_actor_root(door->base_heading+(unsigned)angle,door->hinge,&root);
    BfmBedroomView local=*view;
    for (unsigned k=0;k<3;k++) {
        int64_t translation=0;
        for (unsigned j=0;j<3;j++)
            translation+=(int64_t)view->axes[k][j]*door->hinge[j];
        local.translation[k]+=q12(translation);
        for (unsigned j=0;j<3;j++) {
            int64_t axis=0;
            for (unsigned n=0;n<3;n++) axis+=(int64_t)view->axes[k][n]*root.rotation[n][j];
            local.axes[k][j]=q12(axis);
        }
    }
    int projected[DOOR_VERTICES][3];
    for (unsigned i=0;i<door->vertices;i++)
        bfm_bedroom_project(&local,door->vertex[i],projected[i]);
    for (unsigned i=0;i<door->faces_used;i++) {
        DoorFace *f=&door->faces[i];
        int depth=0;
        for (unsigned k=0;k<4;k++) {
            const int *pos=projected[f->index[k]];
            if (pos[2]<=0) { depth=0; break; }
            f->corners[k].x=pos[0];
            f->corners[k].y=pos[1];
            depth+=pos[2];
        }
        if (!depth) continue;
        const BfmForestRasterVertex *c=f->corners;
        if ((int64_t)(c[1].x-c[0].x)*(c[2].y-c[0].y)<=
            (int64_t)(c[1].y-c[0].y)*(c[2].x-c[0].x)) continue;
        f->depth=depth/16;
        unsigned pos=door->visible++;
        while (pos && door->faces[door->order[pos-1]].depth<=f->depth) {
            door->order[pos]=door->order[pos-1];
            pos--;
        }
        door->order[pos]=i;
    }
}

int bfm_village_mayor_door_next_depth(const BfmVillageMayorDoor *door)
{
    return door && door->next < door->visible ? door->faces[door->order[door->next]].depth : INT_MIN;
}

void bfm_village_mayor_door_draw_to(BfmVillageMayorDoor *door, int depth)
{
    if (!door) return;
    while (door->next < door->visible && bfm_village_mayor_door_next_depth(door)>=depth) {
        DoorFace *f=&door->faces[door->order[door->next++]];
        bfm_field_raster_face(&door->raster,&f->material,f->corners);
    }
}

struct BfmVillageShopDoors {
    BfmVillageMayorDoor *inn, *bakery, *toy_open, *toy_closed, *toy;
};

void bfm_village_shop_doors_free(BfmVillageShopDoors *doors)
{
    if (!doors) return;
    bfm_village_mayor_door_free(doors->inn);
    bfm_village_mayor_door_free(doors->bakery);
    bfm_village_mayor_door_free(doors->toy_open);
    bfm_village_mayor_door_free(doors->toy_closed);
    free(doors);
}

BfmVillageShopDoors *bfm_village_shop_doors_load(const uint8_t *scene, size_t bytes, uint32_t room)
{
    BfmVillageShopDoors *doors = calloc(1,sizeof *doors);
    if (!doors) return NULL;
    doors->bakery = load_leaf(scene,bytes,room,8,0);
    doors->inn = load_leaf(scene,bytes,room,10,0);
    doors->toy_open = load_leaf(scene,bytes,room,13,0);
    doors->toy_closed = load_leaf(scene,bytes,room,13,1);
    if (!doors->bakery || !doors->inn || !doors->toy_open || !doors->toy_closed) {
        bfm_village_shop_doors_free(doors);
        return NULL;
    }
    return doors;
}

void bfm_village_shop_doors_prepare(BfmVillageShopDoors *doors, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels,
                                    const BfmSaveRecord *state)
{
    unsigned minute = u16le(state->progress+0x1a), story = u32le(state->progress+8);
    /* The inn receives guests around the clock. The toy shop opens at noon,
     * closes at eight and takes Wednesdays off, with two story closures. */
    int open = state->progress[0x42] != 2 && minute >= 12 * 60 && minute < 20 * 60 &&
               !(story >= 100 && story < 130) && !(story >= 950 && story < 1000);
    doors->toy = open ? doors->toy_open : doors->toy_closed;
    bfm_village_mayor_door_prepare(doors->bakery,view,tiles,pixels,
                                  bfm_bakery_door_is_open(state) ? -1536 : 0);
    bfm_village_mayor_door_prepare(doors->inn,view,tiles,pixels,-1536);
    bfm_village_mayor_door_prepare(doors->toy,view,tiles,pixels,open ? -1536 : 0);
}

int bfm_village_shop_doors_next_depth(const BfmVillageShopDoors *doors)
{
    int inn = bfm_village_mayor_door_next_depth(doors->inn);
    int toys = bfm_village_mayor_door_next_depth(doors->toy);
    int bakery = bfm_village_mayor_door_next_depth(doors->bakery);
    int next = inn > toys ? inn : toys;
    return next > bakery ? next : bakery;
}

void bfm_village_shop_doors_draw_to(BfmVillageShopDoors *doors, int depth)
{
    int next;
    while ((next = bfm_village_shop_doors_next_depth(doors)) != INT_MIN && next >= depth) {
        bfm_village_mayor_door_draw_to(doors->bakery,next);
        bfm_village_mayor_door_draw_to(doors->inn,next);
        bfm_village_mayor_door_draw_to(doors->toy,next);
    }
}
