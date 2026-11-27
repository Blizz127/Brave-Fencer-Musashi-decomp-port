/* SPDX-License-Identifier: MIT */
#include "valley_gate_people.h"
#include "../pc_port/forest_player_bytes.h"
#include <math.h>
#include <stdlib.h>

enum { PEOPLE = 4, WALK_KEYS = 6, GESTURE_KEYS = 8, UPLOAD_WORDS = 16, UPLOAD_ROWS = 32 };
enum Action { ENTERING, WAITING, HOPPING };
static const uint32_t origin = UINT32_C(0x80128158);

typedef struct {
    const uint8_t *sprite, *images[WALK_KEYS + GESTURE_KEYS];
    int16_t position[3], ground_y;
    uint16_t age;
    unsigned field, key;
    enum Action action;
} Attendant;

struct BfmValleyGatePeople {
    Attendant actors[PEOPLE];
    uint16_t (*tiles)[1024];
    int present;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t length)
{
    if (address < origin || address-origin > bytes || length > bytes-(address-origin)) return NULL;
    return scene + (address-origin);
}

static int load_actor(Attendant *a, const uint8_t *scene, size_t bytes, unsigned role)
{
    const uint8_t *station = mounted(scene, bytes, UINT32_C(0x801862fc) + 20*role, 20);
    const uint8_t *profile = mounted(scene, bytes, UINT32_C(0x80186444) + 100*role, 84);
    if (!station || !profile || u16le(station+6) != 306+role ||
        profile[0] != 255 || profile[1] < 224 || profile[2] != 31 || profile[3] != 31)
        return 0;
    for (unsigned k=0; k<3; ++k) a->position[k] = s16le(station + 2*k);
    if ((role < 2 && a->position[0] < INT16_MIN+320) ||
        (role >= 2 && a->position[0] > INT16_MAX-320) || a->position[1] < INT16_MIN+64) return 0;
    a->ground_y = a->position[1];
    a->sprite = profile;
    unsigned tx = u16le(profile+4), ty = u16le(profile+6);
    if (tx > 1024-UPLOAD_WORDS || ty > 512-UPLOAD_ROWS ||
        2*(tx%64)+profile[2] > 255 || ty%256+profile[3] > 255) return 0;
    for (unsigned i=0; i<WALK_KEYS+GESTURE_KEYS; ++i) {
        const uint8_t *upload = mounted(scene, bytes, u32le(profile+28+4*i), 20);
        if (!upload || u32le(upload) || u16le(upload+4) != tx || u16le(upload+6) != ty ||
            u16le(upload+8) != UPLOAD_WORDS || u16le(upload+10) != UPLOAD_ROWS ||
            u32le(upload+16) != 255) return 0;
        a->images[i] = mounted(scene, bytes, u32le(upload+12), 2*UPLOAD_WORDS*UPLOAD_ROWS);
        if (!a->images[i]) return 0;
    }
    return 1;
}

BfmValleyGatePeople *bfm_valley_gate_people_open(const uint8_t *scene, size_t bytes,
                                                 unsigned entry_story, uint16_t tiles[512][1024])
{
    if (!scene || !tiles) return NULL;
    const uint8_t *room = mounted(scene, bytes, UINT32_C(0x8018231c), 4);
    if (!room || u32le(room) != UINT32_C(0x80186620)) return NULL;
    BfmValleyGatePeople *people = calloc(1, sizeof *people);
    if (!people) return NULL;
    people->tiles = tiles;
    people->present = entry_story == 150;
    for (unsigned i=0; people->present && i<PEOPLE; ++i) {
        if (!load_actor(&people->actors[i], scene, bytes, i)) {
            free(people);
            return NULL;
        }
    }
    return people;
}

void bfm_valley_gate_people_free(BfmValleyGatePeople *people)
{
    free(people);
}

static void upload_frame(const Attendant *a, uint16_t tiles[512][1024])
{
    unsigned tx = u16le(a->sprite+4), ty = u16le(a->sprite+6);
    const uint8_t *source = a->images[a->key];
    for (unsigned row=0; row<UPLOAD_ROWS; ++row)
        for (unsigned col=0; col<UPLOAD_WORDS; ++col, source+=2)
            tiles[ty+row][tx+col] = u16le(source);
}

static int hop_to_plate(Attendant *a, int inward)
{
    /* The airborne sprite starts the gesture and holds its extended frame. */
    a->key = WALK_KEYS + (a->age/2) % GESTURE_KEYS;
    a->age = 8;
    ++a->field;
    int sine = (int)lround(4096*sin(a->field * (3.14159265358979323846/32)));
    a->position[0] += inward*2;
    a->position[1] = (int16_t)(a->ground_y - sine/64);
    if (a->field < 32) return 0;
    a->action = WAITING;
    return 1;
}

int bfm_valley_gate_people_step(BfmValleyGatePeople *people, uint8_t member[4])
{
    if (!people || !member || !people->present) return 0;
    int soldier_landed = 0;
    for (unsigned i=0; i<PEOPLE; ++i) {
        Attendant *a = &people->actors[i];
        int inward = i < 2 ? -1 : 1;
        ++a->age;
        switch (a->action) {
        case ENTERING:
            a->key = (a->age/2) % WALK_KEYS;
            a->position[0] += inward*8;
            if (++a->field == 32) a->action = WAITING;
            break;
        case WAITING:
            if (member[i] == 2) {
                member[i] = 0;
                a->field = a->age = 0;
                a->action = HOPPING;
                continue;
            }
            a->key = member[i] == 1 ? WALK_KEYS + (a->age/2) % GESTURE_KEYS : 0;
            break;
        case HOPPING:
            if (hop_to_plate(a, inward) && !i) soldier_landed = 1;
            break;
        }
        upload_frame(a, people->tiles);
    }
    return soldier_landed;
}

static int add_actor(const Attendant *a, BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    int center[3];
    if (!bfm_bedroom_project(view, a->position, center) || center[2]/4+1 >= 4096) return 1;
    const uint8_t *s = a->sprite;
    /* A type-3 world sprite projects its pivot and span separately at the
     * anchor depth. The gate team uses four times the mounted local extent. */
    BfmBedroomView plane = {.axes={{16384,0,0},{0,16384,0},{0,0,4096}},
                            .translation={0,0,center[2]}, .focal=view->focal};
    const int16_t pivot[3] = {s16le(s+8), s16le(s+10), 0}, extent[3] = {s[2],s[3],0};
    int projected[3], size[3];
    if (!bfm_bedroom_project(&plane, pivot, projected) ||
        !bfm_bedroom_project(&plane, extent, size)) return 1;
    int x = center[0]+projected[0]-160, y = center[1]+projected[1]-120;
    int w = size[0]-160, h = size[1]-120;
    if (w < 1 || h < 1 || x >= 320 || y >= 240 || x+w <= 0 || y+h <= 0) return 1;

    unsigned tx = u16le(s+4), ty = u16le(s+6);
    BfmForestDustCard card = {.bucket=(unsigned)(center[2]/4+1),
        .material={.count=4,.textured=1,.page=(uint16_t)(0x80u | tx/64 | (ty/256)<<4),
                   .clut=(uint16_t)((256u+s[1])*64+256/16)}};
    for (unsigned k=0; k<4; ++k) {
        unsigned right = k&1u, bottom = k>>1;
        card.vertices[k] = (BfmForestRasterVertex){
            .x=x+(int)right*w,.y=y+(int)bottom*h,.u=(uint8_t)(2*(tx%64)+right*s[2]),
            .v=(uint8_t)(ty%256+bottom*s[3]),.rgb=0x808080};
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}

int bfm_valley_gate_people_draw(const BfmValleyGatePeople *people, BfmBedroomFigure *figure,
                                const BfmBedroomView *view)
{
    if (!people || !figure || !view) return 0;
    for (unsigned i=0; people->present && i<PEOPLE; ++i)
        if (!add_actor(&people->actors[i], figure, view)) return 0;
    return 1;
}
