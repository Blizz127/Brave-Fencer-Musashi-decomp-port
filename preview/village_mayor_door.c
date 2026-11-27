/* SPDX-License-Identifier: MIT */
#include "village_mayor_door.h"
#include "bakery_door.h"
#include "forest_pose.h"
#include "../pc_port/forest_player_bytes.h"
#include "forest_raster.h"
#include <limits.h>
#include <stdlib.h>

enum { DOOR_VERTICES = 14, DOOR_FACES = 11, FIELD_BASE = 0x80128158u,
       ILLNESS_LOWER_ROOM = 0x80190780u };

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
                                      const uint8_t *row, int closed)
{
    if (!row) return NULL;
    unsigned building = u16le(row+8);
    uint32_t shapes = room == ILLNESS_LOWER_ROOM ? 0x801bbed0u : 0x801c0b68u;
    uint32_t pairs = room == ILLNESS_LOWER_ROOM ? 0x801bbe90u : 0x801c0b28u;
    const uint8_t *bank = at(scene,bytes,shapes + 2 * building,2);
    if (!bank || u16le(bank) > 1) return NULL;
    const uint8_t *entry = at(scene,bytes,pairs + 8 * u16le(bank) + 4 * closed,4);
    if (!entry) return NULL;
    const uint8_t *models = at(scene,bytes,u32le(entry),8);
    if (!models || u32le(models+4)) return NULL;
    uint32_t model = u32le(models);
    const uint8_t *tmd = at(scene,bytes,model,40);
    if (!tmd || u32le(tmd)!=0x41 || u32le(tmd+4) || u32le(tmd+8)!=1) return NULL;
    const uint8_t *object = tmd+12;
    size_t body = bytes-(size_t)(object-scene);
    uint32_t verts=u32le(object), prims=u32le(object+16);
    unsigned vertices = u32le(object+4), faces = u32le(object+20);
    if (!vertices || vertices > DOOR_VERTICES || !faces || faces > DOOR_FACES ||
        verts > body || vertices * 8 > body - verts || prims > body)
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
        if (body - prims < 4) goto invalid;
        const uint8_t *p = object+prims;
        unsigned count;
        if (p[3] == 0x25 && p[0] == 7 && p[1] == 6) count = 3;
        else if (p[3] == 0x2d && p[0] == 9 && p[1] == 7) count = 4;
        else goto invalid;
        unsigned packet = 4u * (1u + p[1]);
        if (p[2] != 1 || packet > body - prims) goto invalid;
        prims += packet;
        DoorFace *f=&door->faces[i];
        f->material=(BfmForestRasterMaterial){.page=u16le(p+10),.clut=u16le(p+6),.count=count,.textured=1};
        const uint8_t *color = p + 4 + 4*count;
        for (unsigned k=0;k<count;k++) {
            unsigned index=u16le(color+4+2*k);
            if (index>=door->vertices) goto invalid;
            f->index[k]=(uint8_t)index;
            f->corners[k]=(BfmForestRasterVertex){.u=p[4+4*k],.v=p[5+4*k],.rgb=u32le(color)&0xffffff};
        }
    }
    return door;
invalid:
    free(door);
    return NULL;
}

BfmVillageMayorDoor *bfm_village_mayor_door_load(const uint8_t *scene, size_t bytes, uint32_t room)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    return load_leaf(scene,bytes,room,placement(scene,bytes,room,5),0);
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
        for (unsigned k=0;k<f->material.count;k++) {
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
        f->depth=depth/(4*(int)f->material.count);
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

typedef struct {
    unsigned building;
    BfmVillageMayorDoor *open, *closed, *current;
} ShopDoor;

struct BfmVillageShopDoors {
    ShopDoor leaves[13];
    unsigned count;
    int illness;
};

void bfm_village_shop_doors_free(BfmVillageShopDoors *doors)
{
    if (!doors) return;
    for (unsigned i = 0; i < doors->count; ++i) {
        bfm_village_mayor_door_free(doors->leaves[i].open);
        bfm_village_mayor_door_free(doors->leaves[i].closed);
    }
    free(doors);
}

static int add_shop(BfmVillageShopDoors *doors, const uint8_t *scene, size_t bytes,
                    uint32_t room, const uint8_t *row)
{
    if (!row || doors->count == sizeof doors->leaves / sizeof doors->leaves[0]) return 0;
    ShopDoor *leaf = &doors->leaves[doors->count++];
    leaf->building = u16le(row+8);
    leaf->open = load_leaf(scene,bytes,room,row,0);
    if (!leaf->open) return 0;
    if (doors->illness || leaf->building == 13) {
        leaf->closed = load_leaf(scene,bytes,room,row,1);
        if (!leaf->closed) return 0;
    }
    return 1;
}

BfmVillageShopDoors *bfm_village_shop_doors_load(const uint8_t *scene, size_t bytes, uint32_t room)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmVillageShopDoors *doors = calloc(1,sizeof *doors);
    if (!doors) return NULL;
    doors->illness = room == ILLNESS_LOWER_ROOM;
    if (doors->illness) {
        const uint8_t *root = at(scene,bytes,room,0xbc);
        if (!root) goto invalid;
        uint32_t begin = u32le(root+0xb4), end = u32le(root+0xb8);
        if (begin > end || !at(scene,bytes,begin,end-begin)) goto invalid;
        unsigned seen = 0;
        for (uint32_t addr = begin; end-addr >= 20; addr += 20) {
            const uint8_t *row = at(scene,bytes,addr,20);
            unsigned kind = u16le(row+6);
            if (!kind) break;
            if (kind != 97) continue;
            unsigned building = u16le(row+8);
            if (building < 2 || building > 16 || building == 11 || building == 15 ||
                (seen & (1u << building))) goto invalid;
            seen |= 1u << building;
            if (!add_shop(doors,scene,bytes,room,row)) goto invalid;
        }
        if (doors->count != sizeof doors->leaves / sizeof doors->leaves[0]) goto invalid;
    } else {
        const unsigned storefronts[] = {8,10,13};
        for (unsigned i = 0; i < sizeof storefronts / sizeof storefronts[0]; ++i)
            if (!add_shop(doors,scene,bytes,room,placement(scene,bytes,room,storefronts[i])))
                goto invalid;
    }
    return doors;
invalid:
    bfm_village_shop_doors_free(doors);
    return NULL;
}

/* Settled storefront states during ordinary player control. */
static int storefront_open(const ShopDoor *door, const BfmSaveRecord *state, int illness)
{
    unsigned minute = u16le(state->progress+0x1a);
    unsigned hour = state->progress[0x39], weekday = state->progress[0x42];
    unsigned story = u32le(state->progress+8);
    switch (door->building) {
    case 8:
        return bfm_bakery_door_is_open(state);
    case 9:
        return weekday != 0 && hour >= 11 && hour < 16;
    case 10:
        return 1;
    case 12:
        return hour >= 10 && hour < 20 &&
               !(story >= 300 && story < 600 && (state->flags[250/8] & (1u << (250%8))));
    case 13:
        return weekday != 2 && (illness ? hour >= 12 && hour < 20 :
                                          minute >= 12*60 && minute < 20*60) &&
               !(story >= 100 && story < 130) && !(story >= 950 && story < 1000);
    case 14:
        return story >= 200 && story < 600 && hour < 3 &&
               (state->progress[0x38] < 15 ||
                (state->progress[0x38] >= 30 && state->progress[0x38] < 45));
    default:
        return 0;
    }
}

void bfm_village_shop_doors_prepare(BfmVillageShopDoors *doors, const BfmBedroomView *view,
                                    const uint16_t tiles[512][1024], uint32_t *pixels,
                                    const BfmSaveRecord *state)
{
    if (!doors || !state) return;
    for (unsigned i = 0; i < doors->count; ++i) {
        ShopDoor *leaf = &doors->leaves[i];
        int open = storefront_open(leaf,state,doors->illness);
        leaf->current = open || !leaf->closed ? leaf->open : leaf->closed;
        bfm_village_mayor_door_prepare(leaf->current,view,tiles,pixels,open ? -1536 : 0);
    }
}

int bfm_village_shop_doors_next_depth(const BfmVillageShopDoors *doors)
{
    int next = INT_MIN;
    if (!doors) return next;
    for (unsigned i = 0; i < doors->count; ++i) {
        int depth = bfm_village_mayor_door_next_depth(doors->leaves[i].current);
        if (depth > next) next = depth;
    }
    return next;
}

void bfm_village_shop_doors_draw_to(BfmVillageShopDoors *doors, int depth)
{
    int next;
    while ((next = bfm_village_shop_doors_next_depth(doors)) != INT_MIN && next >= depth)
        for (unsigned i = 0; i < doors->count; ++i)
            bfm_village_mayor_door_draw_to(doors->leaves[i].current,next);
}
