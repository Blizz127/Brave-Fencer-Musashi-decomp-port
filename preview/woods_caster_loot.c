/* SPDX-License-Identifier: MIT */
#include "woods_caster_loot.h"
#include "forest_damage_digits.h"
#include "village_ground.h"
#include "woods_capture_light.h"
#include "../pc_port/forest_player_bytes.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { MONEY, HP, BP, KINDS, TIERS = 4, ROOM_SLOTS = 96, NUMBER_SLOTS = 30,
       SPRITE_TICKS = 384, BLINK_TICKS = 48 };
static const uint32_t room = UINT32_C(0x8018ea38);

typedef struct {
    BfmBedroomRoot root;
    int velocity, bounces, life;
    unsigned kind, tier;
} Pickup;

typedef struct {
    BfmForestDamageDigits digits;
    int money;
} PickupNumber;

struct BfmWoodsCasterLoot {
    const uint8_t *scene, *categories, *tiers, *amounts, *palettes, *shadow, *bp_colors;
    const uint8_t *money_frames[TIERS][5], *hp_frames[8];
    size_t bytes;
    BfmVillageGround *ground;
    Pickup pickups[ROOM_SLOTS];
    PickupNumber numbers[NUMBER_SLOTS];
};

static const uint8_t *at(const BfmWoodsCasterLoot *loot, uint32_t address, size_t size)
{
    const uint32_t origin = UINT32_C(0x80128158);
    if (address < origin || address - origin > loot->bytes ||
        size > loot->bytes - (address-origin)) return NULL;
    return loot->scene + (address-origin);
}

static int load_frames(BfmWoodsCasterLoot *loot, uint32_t program, unsigned count,
                       unsigned duration, const uint8_t **frames)
{
    const uint8_t *keys = at(loot, program, 8*(count+1));
    if (!keys || u32le(keys+8*count) || u32le(keys+8*count+4) != 0xffff) return 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *s = at(loot, u32le(keys+8*i), 12);
        if (u32le(keys+8*i+4) != duration || !s || s[0] != 255 || !s[2] || !s[3]) return 0;
        unsigned x = u16le(s+4), y = u16le(s+6);
        if (x >= 1024 || y >= 512 || (x%64)*4+s[2] > 255 || y%256+s[3] > 255) return 0;
        frames[i] = s;
    }
    return 1;
}

BfmWoodsCasterLoot *bfm_woods_caster_loot_open(const uint8_t *scene, size_t bytes)
{
    if (!scene || bytes > BFM_FIELD_SCENE_CAPACITY) return NULL;
    BfmWoodsCasterLoot *loot = calloc(1, sizeof *loot);
    if (!loot) return NULL;
    loot->scene = scene;
    loot->bytes = bytes;
    loot->categories = at(loot, UINT32_C(0x80187c20), 96);
    loot->tiers = at(loot, UINT32_C(0x80187c80), 72);
    loot->amounts = at(loot, UINT32_C(0x80189470), 24);
    loot->palettes = at(loot, UINT32_C(0x8018a974), 5);
    loot->shadow = at(loot, UINT32_C(0x80189128), 12);
    loot->bp_colors = at(loot, UINT32_C(0x801d7844), 8);
    const uint8_t *drop = at(loot, UINT32_C(0x801c48e0), 8);
    const uint8_t *money = at(loot, UINT32_C(0x80188e34), 4*TIERS);
    if (!loot->categories || !loot->tiers || !loot->amounts || !loot->palettes ||
        !loot->shadow || !loot->bp_colors || !money || !drop ||
        u32le(drop) != 21 || u32le(drop+4) != UINT32_C(0x10000)) goto fail;
    for (unsigned i = 0; i < TIERS; ++i)
        if (!load_frames(loot, u32le(money+4*i), 5, 2, loot->money_frames[i])) goto fail;
    if (!load_frames(loot, UINT32_C(0x80188eb4), 8, 1, loot->hp_frames)) goto fail;
    loot->ground = bfm_village_ground_open(scene, bytes, room);
    if (loot->ground) return loot;
fail:
    bfm_woods_caster_loot_free(loot);
    return NULL;
}

void bfm_woods_caster_loot_free(BfmWoodsCasterLoot *loot)
{
    if (loot) bfm_village_ground_free(loot->ground);
    free(loot);
}

static unsigned choose(const uint8_t *limits, unsigned width, unsigned roll)
{
    unsigned i = 0;
    while (i < width && roll >= u16le(limits+2*i)) ++i;
    return i;
}

static unsigned resource_band(unsigned current, unsigned maximum)
{
    if (current > maximum*7/10) return 0;
    if (current > maximum/2) return 1;
    if (current > maximum/5) return 2;
    return 3;
}

static int move_drop(const BfmWoodsCasterLoot *loot, Pickup *item);

int bfm_woods_caster_loot_drop(BfmWoodsCasterLoot *loot, BfmGameplayRun *run,
                               const int16_t caster_root[3])
{
    if (!loot || !run || !caster_root || caster_root[1] < INT16_MIN+32) return 0;
    const uint8_t *p = run->player.record;
    unsigned hp = u16le(p+0x3c), bp = u16le(p+0x40);
    unsigned max_hp = u16le(p+0x3a), max_bp = u16le(p+0x3e);
    unsigned hp_band = hp == max_hp ? 4 : hp <= max_hp/2;
    unsigned bp_band = bp == max_bp ? 8 : bp <= max_bp/2 ? 2 : 0;
    unsigned kind = choose(loot->categories+6*(hp_band+bp_band), KINDS,
                           bfm_gameplay_random(run)%100);
    unsigned offset = kind == HP ? 1+resource_band(hp,max_hp) :
                      kind == BP ? 5+resource_band(bp,max_bp) : 0;
    unsigned roll = bfm_gameplay_random(run)%100;
    if (kind == KINDS) return 1;
    unsigned tier = choose(loot->tiers+8*offset, TIERS, roll);
    if (tier == TIERS) return 0;
    for (unsigned i = 0; i < ROOM_SLOTS; ++i) {
        Pickup *item = &loot->pickups[i];
        if (item->life) continue;
        /* The allocator puts this child after its parent. Its regular
         * actor update still runs on the birth field after the hero scan. */
        Pickup newborn = {.kind=kind, .tier=tier, .velocity=-12,
                          .life=SPRITE_TICKS+BLINK_TICKS-1};
        memcpy(newborn.root.xyz, caster_root, sizeof newborn.root.xyz);
        newborn.root.xyz[1] -= 32;
        if (!move_drop(loot,&newborn)) return 0;
        *item = newborn;
        return 1;
    }
    return 1;
}

static int move_drop(const BfmWoodsCasterLoot *loot, Pickup *item)
{
    if (item->bounces == 2) return 1;
    int16_t next[3];
    memcpy(next, item->root.xyz, sizeof next);
    item->velocity += 2;
    int y = next[1]+item->velocity;
    if (y < INT16_MIN || y > INT16_MAX) return 0;
    next[1] = (int16_t)y;
    BfmForestContact floor;
    BfmVillageGroundResult result = bfm_village_ground_air(loot->ground,item->root.xyz,next,&floor);
    if (result == BFM_VILLAGE_GROUND_BLOCKED) return 0;
    memcpy(item->root.xyz, next, sizeof next);
    if (item->velocity > 24) item->velocity = 24;
    if (next[1] > 256 || (result == BFM_VILLAGE_GROUND_FOOTING && (floor.slope & 0x6000))) {
        if (!item->bounces) item->velocity = -8;
        ++item->bounces;
    }
    return bfm_bedroom_travel_air_shadow(loot->scene, loot->bytes, room, &item->root);
}

static int in_reach(const int16_t a[3], const int16_t b[3])
{
    int radius_squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        int d = a[i]-b[i];
        if (d <= -48 || d >= 48) return 0;
        radius_squared += d*d;
    }
    return radius_squared < 48*48;
}

static unsigned collect(BfmWoodsCasterLoot *loot, Pickup *item, BfmForestStatus *player,
                        const int16_t hero[3])
{
    unsigned amount = u16le(loot->amounts+2*(TIERS*item->kind+item->tier));
    uint8_t *p = player->record;
    item->life = 0;
    unsigned style = 3;
    if (item->kind == MONEY) {
        uint32_t wallet = u32le(p+0x14);
        wallet = wallet > 99999-amount ? 99999 : wallet+amount;
        for (unsigned k = 0; k < 4; ++k) p[0x14+k] = (uint8_t)(wallet >> (8*k));
    } else {
        unsigned offset = item->kind == HP ? 0x3c : 0x40;
        unsigned current = u16le(p+offset), maximum = u16le(p+offset-2);
        if (item->kind == HP && !current) return 0;
        current += amount;
        if (current > maximum) current = maximum;
        p[offset] = (uint8_t)current;
        p[offset+1] = (uint8_t)(current >> 8);
        style = item->kind == HP ? 2 : 4;
    }
    for (unsigned i = 0; i < NUMBER_SLOTS; ++i) {
        PickupNumber *number = &loot->numbers[i];
        if (number->digits.amount) continue;
        number->money = item->kind == MONEY;
        unsigned display_amount = number->money ? 10*amount : amount;
        if (display_amount && display_amount < 1000 &&
            bfm_forest_damage_digits_start_data(&number->digits,loot->palettes+style,
                                                hero,(uint16_t)display_amount))
            number->digits.bp_credit = 1;
        break;
    }
    return 1106;
}

int bfm_woods_caster_loot_step(BfmWoodsCasterLoot *loot, BfmGameplayRun *run,
                               const int16_t hero[3], int allow_pickup, unsigned *sound)
{
    if (!loot || !run || !hero || !sound) return 0;
    *sound = 0;
    for (unsigned i = 0; i < NUMBER_SLOTS; ++i)
        bfm_forest_damage_digits_advance(&loot->numbers[i].digits);
    for (unsigned i = 0; i < ROOM_SLOTS; ++i) {
        Pickup *item = &loot->pickups[i];
        if (!item->life) continue;
        if (allow_pickup && in_reach(hero,item->root.xyz)) {
            *sound = collect(loot,item,&run->player,hero);
            allow_pickup = 0;
        } else {
            if (item->life > BLINK_TICKS && !move_drop(loot,item)) return 0;
            --item->life;
        }
    }
    return 1;
}

static int projected(int local, int scale, int reciprocal)
{
    int scaled = local*scale;
    int64_t q = (int64_t)(scaled/4096-(scaled%4096 < 0))*reciprocal;
    return (int)(q/65536-(q%65536 < 0));
}

static int add_pickup(const BfmWoodsCasterLoot *loot, const Pickup *item, unsigned tick,
                      BfmBedroomFigure *figure, const BfmBedroomView *view,
                      const BfmForestCamera *camera)
{
    int scale = 3072+1536*(int)item->tier;
    /* This actor is a billboard with a separate ground-shadow child. Submit
     * the shadow without body polygons, including during the final blinks. */
    const MusashiForestPlayerPart billboard = {0};
    if (item->root.shadow_visible && !bfm_bedroom_figure_add_morph(figure,view,&billboard,
            &item->root,4096,4096,loot->shadow,(unsigned)scale)) return 0;
    if (item->life < BLINK_TICKS && (item->life & 1)) return 1;
    int p[3];
    if (!bfm_bedroom_project(view,item->root.xyz,p) || p[2]/4+1 >= 4096) return 1;
    int64_t ratio = ((int64_t)view->focal*65536+p[2]/2)/p[2];
    int reciprocal = ratio > 131071 ? 131071 : (int)ratio;
    BfmForestDustCard cards[6];
    size_t count;
    if (item->kind == BP) {
        const int32_t center[3] = {item->root.xyz[0],item->root.xyz[1],item->root.xyz[2]};
        unsigned half = (tick & 15u)/2u, pulse = (tick & 16u) ? 9+half : 16-half;
        uint32_t rim = 0, hub = 0;
        for (unsigned k = 0; k < 3; ++k) {
            rim |= (uint32_t)loot->bp_colors[k] << (8*k);
            hub |= (uint32_t)(pulse*(loot->bp_colors[4+k]>>4)) << (8*k);
        }
        count = bfm_woods_radial_light_project(camera,center,16,scale,hub,cards,6);
        int lift = projected(-16,scale,reciprocal);
        for (size_t i = 0; i < count; ++i) {
            for (unsigned k = 0; k < 4; ++k) {
                if (k != 2) cards[i].vertices[k].rgb = rim;
                cards[i].vertices[k].y += lift;
            }
        }
    } else {
        unsigned age = SPRITE_TICKS+BLINK_TICKS-(unsigned)item->life;
        const uint8_t *s = item->kind == HP ? loot->hp_frames[age%8] :
                           loot->money_frames[item->tier][age/2%5];
        int x = p[0]+projected(s16le(s+8),scale,reciprocal);
        int y = p[1]+projected(s16le(s+10),scale,reciprocal);
        int w = projected(s[2],scale,reciprocal), h = projected(s[3],scale,reciprocal);
        if (!w || !h || x >= 320 || y >= 240 || x+w <= 0 || y+h <= 0) return 1;
        unsigned tx = u16le(s+4), ty = u16le(s+6);
        unsigned palette_x = s[1] < 224 ? 352 : 256;
        cards[0] = (BfmForestDustCard){.bucket=(unsigned)(p[2]/4+1),
            .material={.textured=1,.count=4,.page=(uint16_t)(tx/64 | (ty/256)<<4),
                       .clut=(uint16_t)((256u+s[1])*64+palette_x/16)}};
        for (unsigned k = 0; k < 4; ++k) {
            int right = k&1, bottom = k>>1;
            cards[0].vertices[k] = (BfmForestRasterVertex){x+right*w, y+bottom*h,
                (uint8_t)(4*(tx%64)+right*s[2]), (uint8_t)(ty%256+bottom*s[3]), 0x808080};
        }
        count = 1;
    }
    return !count || bfm_bedroom_figure_add_fusion(figure,view,NULL,cards,count);
}

static int add_number(const PickupNumber *number, BfmBedroomFigure *figure,
                      const BfmBedroomView *view, const BfmForestCamera *camera)
{
    BfmForestDustCard cards[BFM_FOREST_DAMAGE_DIGIT_CARDS];
    unsigned count = bfm_forest_damage_digits_project(&number->digits,camera,cards);
    if (!count) return 1;
    if (number->money) {
        if (count >= BFM_FOREST_DAMAGE_DIGIT_CARDS) return 0;
        /* Include the currency symbol in the centered run. The shared
         * number projector already placed the trailing zero. */
        for (unsigned i = 0; i < count; ++i)
            for (unsigned k = 0; k < 4; ++k) cards[i].vertices[k].x -= 4;
        cards[count] = cards[count-1];
        cards[count].slot = count;
        for (unsigned k = 0; k < 4; ++k) {
            cards[count].vertices[k].x -= 8;
            cards[count].vertices[k].u = (uint8_t)(32+8*(k&1));
        }
        ++count;
    }
    return bfm_bedroom_figure_add_fusion(figure,view,NULL,cards,count);
}

int bfm_woods_caster_loot_add(const BfmWoodsCasterLoot *loot, BfmBedroomFigure *figure,
                              const BfmBedroomView *view, unsigned field_tick)
{
    if (!loot || !figure || !view || view->focal <= 0 || view->focal > UINT16_MAX) return 0;
    BfmForestCamera camera = {.projection=(uint16_t)view->focal};
    for (unsigned row = 0; row < 3; ++row) {
        camera.translation[row] = view->translation[row];
        for (unsigned col = 0; col < 3; ++col) {
            int v = view->axes[row][col];
            if (v < INT16_MIN || v > INT16_MAX) return 0;
            camera.rotation[row][col] = (int16_t)v;
        }
    }
    for (unsigned i = 0; i < ROOM_SLOTS; ++i)
        if (loot->pickups[i].life &&
            !add_pickup(loot,&loot->pickups[i],field_tick,figure,view,&camera)) return 0;
    for (unsigned i = 0; i < NUMBER_SLOTS; ++i)
        if (loot->numbers[i].digits.amount &&
            !add_number(&loot->numbers[i],figure,view,&camera)) return 0;
    return 1;
}
