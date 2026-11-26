/* SPDX-License-Identifier: MIT */
#include "tower_summit_arrival.h"
#include "forest_dust.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_bytes.h"

enum { WAIT_FOR_ENTRY, ARRIVING, START_SPEECH, SPEAKING, DISMISS, FREE };
/* The light expands on the floor, extends upward, restores the body to its
 * usual proportions, and then contracts. Phase changes consume one update. */
enum { EXPAND_END = 12, EXTEND_START = 14, REVEAL = 46, RECOVER_END = 78,
       CONTRACT_START = 80, LIGHT_END = 90 };

int bfm_tower_summit_arrival_begin(BfmTowerSummitArrival *arrival, const BfmForestField *room,
                                   const MusashiDiscMedia *disc, uint32_t parts, BfmForestPose *pose)
{
    if (!arrival || !room || !disc || !pose) return 0;
    const uint8_t *door = bfm_forest_field_entrance(room, 0);
    if (!door || u16le(door) != 0x300b) return 0;
    uint32_t start = u32le(door + 12);
    const uint32_t limit = UINT32_C(0x801a7cb0);
    if (start >= limit) return 0;
    BfmTowerSummitArrival fresh = {.scale = {4096, 4096, 4096}, .speech_bytes = limit - start};
    fresh.speech = bfm_forest_room_bytes(room, start, fresh.speech_bytes);
    fresh.rim = bfm_forest_room_bytes(room, UINT32_C(0x8018e5e8), 42);
    if (!fresh.speech || !fresh.rim ||
        !bfm_forest_speech_actor_begin(&fresh.actor, room, disc, parts, pose)) return 0;
    *arrival = fresh;
    return 1;
}

static int radius(unsigned age)
{
    if (age <= EXPAND_END) return (int)age * 224;
    int extent = EXPAND_END * 224;
    if (age >= CONTRACT_START) extent -= (int)(age - CONTRACT_START + 1) * 256;
    return extent > 0 ? extent : 0;
}

int bfm_tower_summit_arrival_tick(BfmTowerSummitArrival *arrival, ForestVoice *voice, unsigned *generation,
                                  int entrance_ready, int confirm, int reveal, BfmForestPose *pose)
{
    if (!arrival || !arrival->actor.field || !pose) return BFM_SUMMIT_ARRIVAL_ERROR;
    if (arrival->phase == FREE) return BFM_SUMMIT_ARRIVAL_RELEASED;
    if (!bfm_forest_speech_motion_tick(&arrival->actor, 0, pose)) return BFM_SUMMIT_ARRIVAL_ERROR;
    switch (arrival->phase) {
    case WAIT_FOR_ENTRY:
        if (entrance_ready) arrival->phase = ARRIVING;
        break;
    case ARRIVING: {
        unsigned age = ++arrival->age;
        if (age >= REVEAL) {
            unsigned t = age < RECOVER_END ? age - REVEAL : RECOVER_END - REVEAL;
            arrival->parts = arrival->actor.visible;
            arrival->scale[0] = arrival->scale[2] = (int16_t)(t * 128);
            arrival->scale[1] = (int16_t)(12288 - t * 256);
        }
        if (age == LIGHT_END) arrival->phase = START_SPEECH;
        break;
    }
    case START_SPEECH:
        arrival->page = bfm_field_message_borrow(arrival->speech, arrival->speech_bytes);
        if (!arrival->page) return BFM_SUMMIT_ARRIVAL_ERROR;
        arrival->phase = SPEAKING;
        break;
    case SPEAKING: {
        if (!voice || !generation) return BFM_SUMMIT_ARRIVAL_ERROR;
        const BfmForestMessageFrame *frame = bfm_forest_voice_page(voice, arrival->page, generation, confirm, reveal);
        if (!frame) return BFM_SUMMIT_ARRIVAL_ERROR;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) arrival->phase = DISMISS;
        break;
    }
    case DISMISS:
        arrival->phase = FREE;
        return BFM_SUMMIT_ARRIVAL_RELEASED;
    }
    return BFM_SUMMIT_ARRIVAL_LOCKED;
}

void bfm_tower_summit_arrival_close(BfmTowerSummitArrival *arrival)
{
    if (arrival) {
        bfm_forest_message_free(arrival->page);
        *arrival = (BfmTowerSummitArrival){0};
    }
}

BfmForestMessageArt *bfm_tower_summit_arrival_art(const BfmForestField *room, const MusashiDiscMedia *disc)
{
    size_t size = 0;
    const uint8_t *scene = bfm_forest_field_scene(room, &size);
    return bfm_castle_message_art_load(disc, scene, size, bfm_forest_field_tiles(room), 0x63174);
}

void bfm_tower_summit_arrival_draw(const BfmTowerSummitArrival *arrival, const BfmForestMessageArt *art,
                                  int speaker_x, uint32_t pixels[240][320])
{
    if (arrival && arrival->page)
        bfm_bedroom_message_art_draw_side(art, bfm_forest_message_frame(arrival->page), speaker_x, 0, pixels);
}

static int scaled(int value, int q12)
{
    int n = value * q12;
    return n / 4096 - (n % 4096 < 0);
}

typedef struct { int16_t x, y, z; } LightPoint;

static void draw_strip(const BfmForestCamera *camera, const BfmForestRaster *raster, uint32_t color,
                       LightPoint a, LightPoint b, LightPoint c, LightPoint d, int fade)
{
    const int16_t points[4][3] = {{a.x,a.y,a.z},{b.x,b.y,b.z},{c.x,c.y,c.z},{d.x,d.y,d.z}};
    BfmForestDustCard card = {0};
    if (!bfm_forest_project_effect_quad(camera, points, &card)) return;
    for (unsigned i = 0; i < 4; ++i) card.vertices[i].rgb = (i == 1 || (i == 0 && fade)) ? 0 : color;
    const BfmForestRasterMaterial material = {.page = 1u << 5, .count = 4, .translucent = 1};
    bfm_forest_raster_face(raster, &material, card.vertices);
}

void bfm_tower_summit_arrival_draw_light(const BfmTowerSummitArrival *arrival, const BfmForestCamera *camera,
                                        const int16_t xyz[3], unsigned parity, uint32_t pixels[240][320])
{
    if (!arrival || arrival->phase != ARRIVING || !camera || !xyz || !pixels) return;
    unsigned age = arrival->age;
    int size = radius(age);
    if (!size) return;
    int height = age < EXTEND_START ? 0 : age >= REVEAL ? -512 : -(int)(age - EXTEND_START + 1) * 16;
    unsigned light = 32u + (unsigned)size / 32u;
    uint32_t color = light << 16 | light << 8 | (parity & 1u ? light / 2u : light);
    LightPoint center = {xyz[0], (int16_t)(xyz[1] - 8), xyz[2]};
    LightPoint inner[21], outer[21], upper[21];
    for (unsigned i = 0; i < 21; ++i) {
        int x = (int8_t)arrival->rim[2*i], z = (int8_t)arrival->rim[2*i+1];
        inner[i] = (LightPoint){center.x + scaled(x,size), center.y, center.z + scaled(z,size)};
        outer[i] = (LightPoint){center.x + scaled(2*x,size), center.y, center.z + scaled(2*z,size)};
        upper[i] = inner[i];
        upper[i].y += (int16_t)scaled(height,size);
    }
    const BfmForestRaster raster = {.pixels = pixels, .bottom = 239};
    for (unsigned i = 0; i < 20; i += 2)
        draw_strip(camera, &raster, color, inner[i], center, inner[i+1], inner[i+2], 0);
    for (unsigned i = 0; i < 20; ++i)
        draw_strip(camera, &raster, color, outer[i], outer[i+1], inner[i], inner[i+1], 1);
    if (height)
        for (unsigned i = 0; i < 20; ++i)
            draw_strip(camera, &raster, color, upper[i], upper[i+1], inner[i], inner[i+1], 1);
}
