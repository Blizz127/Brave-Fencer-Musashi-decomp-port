/* SPDX-License-Identifier: MIT */
#include "woods_satiate_effect.h"
#include "woods_capture_light.h"
#include "forest_bullet_art.h"
#include "forest_shot_flash.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { SATIATE = 3, FOODS = 8, SLOTS = 30, SECTOR = 2048, UNIT = 4096 };

typedef struct {
    unsigned texture, yaw;
    int radius, scale, newborn;
    int32_t position[3];
} Food;

typedef struct {
    unsigned word_x, y, palette_x, palette_y;
    uint16_t palette[16];
} Picture;

struct BfmWoodsSatiateEffect {
    Picture picture[FOODS];
    uint8_t sheet[4 * SECTOR];
    BfmForestDust dust;
    Food food[SLOTS];
    BfmForestShotFlash flash[SLOTS];
    int active, horizontal, vertical;
    unsigned phase, delay, burst, angle, next_food;
};

static int load_food(BfmWoodsSatiateEffect *s, const MusashiDiscMedia *disc)
{
    uint8_t directory[SECTOR], section[SECTOR], layout[FOODS * 4], palettes[FOODS * 64];
    size_t total;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, directory, sizeof directory, &total))
        return 0;
    unsigned count = u32le(directory);
    if (count <= 18 || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = directory + 8 + 18 * 8;
    unsigned sector = u32le(entry), length = u32le(entry + 4);
    if (!sector || sector > total/SECTOR || length > total - (size_t)sector*SECTOR ||
        length < 23 * SECTOR) return 0;
    size_t member = (size_t)sector * SECTOR;
    if (!musashi_disc_media_read_root_range(disc,"MAIN.CD",member+4*SECTOR,section,sizeof section,&total) ||
        memcmp(section,"PAC",4) || u32le(section+4) != 1 ||
        u32le(section+12) < 0x1058 + sizeof palettes || u32le(section+12) > 3*SECTOR ||
        !musashi_disc_media_read_root_range(disc,"MAIN.CD",member+0x2fec,layout,sizeof layout,&total) ||
        !musashi_disc_media_read_root_range(disc,"MAIN.CD",member+0x3058,palettes,sizeof palettes,&total))
        return 0;
    /* Each food has its own texture cell and palette upload in the mounted skill. */
    for (unsigned i = 0; i < FOODS; ++i) {
        Picture *p = &s->picture[i];
        const uint8_t *colors = palettes + 64*i, *upload = colors + 32;
        p->word_x = u16le(layout + 4*i);
        p->y = u16le(layout + 4*i + 2);
        p->palette_x = u16le(upload+4);
        p->palette_y = u16le(upload+6);
        if (p->word_x < 416 || p->word_x+8 > 448 || p->y < 256 || p->y+32 > 384 ||
            u16le(upload) != 9 || u16le(upload+2) != 25+i ||
            p->palette_x != 352 || p->palette_y != 281+i ||
            u16le(upload+8) != 16 || u16le(upload+10) != 1 ||
            u32le(upload+12) != UINT32_C(0x800cb660) + 64*i || u32le(upload+16) != 255)
            return 0;
        for (unsigned j = 0; j < 16; ++j) p->palette[j] = u16le(colors + 2*j);
    }
    if (!musashi_disc_media_read_root_range(disc,"MAIN.CD",member+18*SECTOR,section,sizeof section,&total) ||
        memcmp(section,"PAC",4) || u32le(section+4) || u32le(section+8) != 4 ||
        u32le(section+12) != 5*SECTOR) return 0;
    for (unsigned row = 0; row < 16; ++row)
        if (u32le(section+16+4*row) != ((row >= 8 && row < 12) ? 1u << 13 : 0u))
            return 0;
    return musashi_disc_media_read_root_range(disc,"MAIN.CD",member+19*SECTOR,
                                              s->sheet,sizeof s->sheet,&total);
}

BfmWoodsSatiateEffect *bfm_woods_satiate_effect_load(const MusashiDiscMedia *disc)
{
    BfmWoodsSatiateEffect *s = calloc(1,sizeof *s);
    if (!s) return NULL;
    if (!load_food(s,disc) || !bfm_forest_dust_load(&s->dust,disc)) {
        free(s);
        return NULL;
    }
    s->horizontal = s->vertical = UNIT;
    return s;
}

void bfm_woods_satiate_effect_free(BfmWoodsSatiateEffect *s)
{
    free(s);
}

static void upload(const BfmWoodsSatiateEffect *s, uint16_t tiles[512][1024])
{
    for (unsigned row = 0; row < 128; ++row)
        for (unsigned x = 0; x < 32; ++x)
            tiles[256+row][416+x] = u16le(s->sheet + 2*(row*32+x));
    for (unsigned i = 0; i < FOODS; ++i) {
        const Picture *p = &s->picture[i];
        memcpy(&tiles[p->palette_y][p->palette_x],p->palette,sizeof p->palette);
    }
}

static int floor_q12(int64_t n)
{
    return (int)(n / UNIT - (n % UNIT < 0));
}

static void start_flash(BfmWoodsSatiateEffect *s, const int32_t position[3], const int32_t eye[3])
{
    for (unsigned i = 0; i < SLOTS; ++i) {
        if (s->flash[i].cell) continue;
        BfmForestTransform facing;
        if (!bfm_forest_glow_facing(eye,position,6,&facing)) return;
        /* The independent impact runs on the next field. Seed its position
         * before the first rise and texture key, even if Satiate ends then. */
        s->flash[i] = (BfmForestShotFlash){
            .x = (int16_t)facing.translation[0], .z = (int16_t)facing.translation[2],
            .height_q16 = (uint32_t)facing.translation[1] << 16,
            .velocity_q16 = -4*65536, .cell = BFM_FOREST_DUST_CELLS
        };
        return;
    }
}

static void advance_food(BfmWoodsSatiateEffect *s, Food *food, const int16_t player[3],
                         const int32_t eye[3])
{
    if (!food->radius) return;
    food->radius -= 4;
    if (food->radius < 28) {
        start_flash(s,food->position,eye);
        food->radius = 0;
        s->horizontal += 1024;
        s->vertical -= 256;
        return;
    }
    food->scale -= 44;
    food->yaw = (food->yaw + 10485760u / (804u * (unsigned)food->radius)) & 4095;
    BfmForestTransform turn;
    bfm_forest_actor_root(food->yaw,player,&turn);
    food->position[0] = player[0] + floor_q12((int)turn.rotation[0][2]*food->radius);
    food->position[1] = player[1] - 28 - food->radius / 2;
    food->position[2] = player[2] + floor_q12((int)turn.rotation[2][2]*food->radius);
}

static void start_food(BfmWoodsSatiateEffect *s)
{
    /* Food children occupy the high end of the room's effect pool. */
    for (unsigned i = SLOTS; i-- > 0; ) {
        if (s->food[i].radius) continue;
        s->food[i] = (Food){.texture = s->next_food, .yaw = s->angle, .radius = 448,
                            .scale = 2*UNIT, .newborn = 1};
        s->next_food = (s->next_food + 1) % FOODS;
        return;
    }
    s->burst = 1;
}

unsigned bfm_woods_satiate_effect_tick(BfmWoodsSatiateEffect *s, BfmGameplayRun *run,
    const int16_t player[3], const int32_t eye[3], unsigned field_tick, unsigned equipped,
    int armed, uint16_t tiles[512][1024])
{
    if (!s || !run || !player || !eye || !tiles) return 0;
    for (unsigned i = 0; i < SLOTS; ++i) {
        BfmForestShotFlash *flash = &s->flash[i];
        if (flash->cell == BFM_FOREST_DUST_CELLS) {
            flash->cell = 1;
            flash->height_q16 += (uint32_t)flash->velocity_q16;
            flash->velocity_q16 = -7*32768;
        } else if (flash->cell) bfm_forest_shot_flash_advance(flash);
    }
    if (equipped != SATIATE) {
        if (s->active) {
            memset(s->food,0,sizeof s->food);
            s->active = 0;
            s->horizontal = s->vertical = UNIT;
        }
        return 0;
    }
    if (armed && !s->active) {
        upload(s,tiles);
        s->active = 1;
        s->phase = s->next_food = 0;
        return 0;
    }
    if (!s->active) return 0;
    if (!s->phase) {
        unsigned random = bfm_gameplay_random(run);
        s->angle = random & 4095;
        s->burst = 2 + (random & 1);
        s->phase = 1;
        s->delay = 0;
    } else if (s->phase == 1) {
        if ((s->delay++ & 3) == 0) {
            start_food(s);
            if (!--s->burst) {
                s->phase = 2;
                s->delay = 16;
            }
        }
    } else if (!--s->delay) {
        s->phase = 0;
    }
    if (s->horizontal > UNIT) {
        s->horizontal -= 224;
        s->vertical += 56;
    }
    for (unsigned i = 0; i < SLOTS; ++i) {
        Food *food = &s->food[i];
        /* The birth update allocates a hidden food and halo. Both first
         * acquire their world position on the following field. */
        if (food->newborn) food->newborn = 0;
        else advance_food(s,food,player,eye);
    }
    return (field_tick & 15u) ? 0 : 0x5fb;
}

void bfm_woods_satiate_effect_scale(const BfmWoodsSatiateEffect *s, int xyz_q12[3])
{
    if (!xyz_q12) return;
    xyz_q12[0] = xyz_q12[2] = s ? s->horizontal : UNIT;
    xyz_q12[1] = s ? s->vertical : UNIT;
}

static int project_food(const BfmWoodsSatiateEffect *s, const Food *food,
                        const BfmForestCamera *camera, BfmForestDustCard *out)
{
    const BfmForestCamera screen = {
        .rotation={{UNIT,0,0},{0,UNIT,0},{0,0,UNIT}}, .projection=camera->projection
    };
    int16_t points[4][3] = {{0}};
    for (unsigned i = 0; i < 3; ++i) {
        int64_t sum = 0;
        for (unsigned k = 0; k < 3; ++k)
            sum += (int64_t)camera->rotation[i][k]*food->position[k];
        int64_t v = camera->translation[i] + (int64_t)floor_q12(sum);
        if (v < INT16_MIN || v > INT16_MAX) return 0;
        points[0][i] = (int16_t)v;
    }
    /* Center, local pivot and image dimensions are projected separately. */
    for (unsigned i = 1; i < 4; ++i) points[i][2] = points[0][2];
    points[1][0] = (int16_t)floor_q12(s->dust.left*food->scale);
    points[1][1] = (int16_t)floor_q12(s->dust.top*food->scale);
    points[2][0] = (int16_t)floor_q12(s->dust.width*food->scale);
    points[2][1] = (int16_t)floor_q12(s->dust.height*food->scale);
    BfmForestDustCard card = {0};
    if (!bfm_forest_project_effect_quad(&screen,points,&card) || ++card.bucket >= 4096) return 0;
    const BfmForestRasterVertex *v = card.vertices;
    int x = v[0].x + v[1].x - v[3].x, y = v[0].y + v[1].y - v[3].y;
    int width = v[2].x - v[3].x, height = v[2].y - v[3].y;
    if (width <= 0 || height <= 0) return 0;
    const Picture *p = &s->picture[food->texture];
    card.material = (BfmForestRasterMaterial){.count=4,.textured=1,.translucent=1,
        .page=(uint16_t)(p->word_x/64 | (p->y/256)<<4),
        .clut=(uint16_t)(p->palette_y*64 + p->palette_x/16)};
    for (unsigned i = 0; i < 4; ++i) {
        unsigned right = i&1, bottom = i/2;
        card.vertices[i] = (BfmForestRasterVertex){
            x + (int)right*width, y + (int)bottom*height,
            (uint8_t)((p->word_x%64)*4 + right*(s->dust.width-1)),
            (uint8_t)(p->y%256 + bottom*(s->dust.height-1)), UINT32_C(0x808080)
        };
    }
    *out = card;
    return 1;
}

int bfm_woods_satiate_effect_add(const BfmWoodsSatiateEffect *s,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!s || !figure || !view || view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    BfmForestCamera camera = {.projection = (uint16_t)view->focal};
    for (unsigned i = 0; i < 3; ++i) {
        camera.translation[i] = view->translation[i];
        for (unsigned j = 0; j < 3; ++j) {
            int axis = view->axes[i][j];
            if (axis < INT16_MIN || axis > INT16_MAX) return 0;
            camera.rotation[i][j] = (int16_t)axis;
        }
    }
    for (unsigned i = 0; i < SLOTS; ++i) {
        BfmForestDustCard cards[7];
        size_t count = 0;
        const Food *food = &s->food[i];
        if (food->radius && food->radius < 448) {
            count = bfm_woods_radial_light_project(&camera,food->position,30,food->scale,
                                                    UINT32_C(0x70a0a0),cards,6);
            for (size_t k = 0; k < count; ++k) --cards[k].bucket;
            count += (size_t)project_food(s,food,&camera,&cards[count]);
        }
        if (count && !bfm_bedroom_figure_add_fusion(figure,view,NULL,cards,count)) return 0;
        const BfmForestShotFlash *flash = &s->flash[i];
        if (flash->cell && flash->cell < BFM_FOREST_DUST_CELLS &&
            bfm_forest_shot_flash_project(flash,&s->dust,&camera,cards)) {
            for (unsigned k = 0; k < 4; ++k) cards[0].vertices[k].rgb = UINT32_C(0xc0c0c0);
            if (!bfm_bedroom_figure_add_fusion(figure,view,NULL,cards,1)) return 0;
        }
    }
    return 1;
}
