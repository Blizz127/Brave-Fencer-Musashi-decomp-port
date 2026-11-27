/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_scene_art.h"
#include "woods_earth_shrine_burst.h"
#include "woods_earth_shrine_fragments.h"
#include "hero_smoke_material.h"
#include "../pc_port/forest_player_bytes.h"
#include "woods_earth_shrine_view.h"
#include <stdlib.h>
#include <string.h>

/* Private to the 3026 shrine art translation unit. Original field effect
 * records and their drawable records have independently recycled slots. */
enum { EARTH_DUST_EFFECTS = 128, EARTH_DUST_DRAW_SLOTS = 130 };
typedef struct {
    int16_t origin[3];
    int16_t scale;
    uint8_t phase;         /* 0 vacant, 1 queued, 2..11 resident cells 0..9 */
    uint8_t draw_index;
} EarthScrollGrain;

typedef struct {
    BfmHeroSmokeMaterial material;
    EarthScrollGrain grain[EARTH_DUST_EFFECTS];
    uint8_t draw_owner[EARTH_DUST_DRAW_SLOTS]; /* one-based effect index */
} EarthScrollDust;

static int earth_scroll_dust_load(EarthScrollDust *dust, const uint8_t *main_data, size_t bytes)
{
    EarthScrollDust next = {0};
    if (!dust || !bfm_hero_run_smoke_load(&next.material, main_data, bytes)) return 0;
    for (unsigned i = 0; i < BFM_HERO_SMOKE_CELLS; ++i)
        if (next.material.tracks[0][i].fields != 1) return 0;
    *dust = next;
    return 1;
}

/* Call on each BFM_EARTH_DUST signal, before the enabled effect sweep,
 * using the real global main-loop counter and the just-updated scroll. */
static unsigned earth_scroll_dust_emit(EarthScrollDust *dust, const int16_t owner_xyz[3], unsigned main_tick)
{
    if (!dust || !dust->material.width || !owner_xyz) return 0;
    unsigned allocated = 0, next_slot = 0;
    for (int arm = 0; arm < 4; ++arm) {
        while (next_slot < EARTH_DUST_EFFECTS && dust->grain[next_slot].phase) ++next_slot;
        if (next_slot == EARTH_DUST_EFFECTS) break;
        int dx = -42 + 24 * arm + 3 * (int)(main_tick & 4u);
        dust->grain[next_slot++] = (EarthScrollGrain){
            .origin = {(int16_t)(owner_xyz[0] + dx), (int16_t)(owner_xyz[1] - 32),
                       (int16_t)(owner_xyz[2] - 40)},
            .scale = (int16_t)(4096 + 128 * (dx < 0 ? -dx : dx)), .phase = 1
        };
        ++allocated;
    }
    return allocated;
}

static void earth_scroll_dust_step(EarthScrollDust *dust)
{
    if (!dust) return;
    for (unsigned slot = 0; slot < EARTH_DUST_EFFECTS; ++slot) {
        EarthScrollGrain *grain = &dust->grain[slot];
        if (!grain->phase) continue;
        if (grain->phase == 1) {
            unsigned draw = 0;
            while (draw < EARTH_DUST_DRAW_SLOTS && dust->draw_owner[draw]) ++draw;
            if (draw == EARTH_DUST_DRAW_SLOTS) {
                grain->phase = 0;
                continue;
            }
            grain->draw_index = (uint8_t)draw;
            dust->draw_owner[draw] = (uint8_t)(slot + 1);
        } else if (grain->phase == 1 + BFM_HERO_SMOKE_CELLS) {
            dust->draw_owner[grain->draw_index] = 0;
            grain->phase = 0;
            continue;
        }
        ++grain->phase;
    }
}

static int earth_scroll_dust_draw(const EarthScrollDust *dust, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view)
{
    if (!dust || !dust->material.width || !figure || !view || view->focal <= 0 ||
        view->focal > INT16_MAX) return 0;
    BfmForestDustCard cards[EARTH_DUST_EFFECTS];
    unsigned count = 0;
    /* The figure queues ascending draw slots and reverses equal-depth order. */
    for (unsigned draw = 0; draw < EARTH_DUST_DRAW_SLOTS; ++draw) {
        if (!dust->draw_owner[draw]) continue;
        const EarthScrollGrain *grain = &dust->grain[dust->draw_owner[draw] - 1];
        int age = grain->phase - 2;
        const BfmHeroSmokeFrame *frame = bfm_hero_smoke_frame(&dust->material, 0, (unsigned)age);
        if (!frame) return 0;
        int16_t world[3] = {grain->origin[0], (int16_t)(grain->origin[1] - 2 * age),
                            (int16_t)(grain->origin[2] - 3 * age)};
        int center[3];
        if (!bfm_bedroom_project(view, world, center) || center[2] <= view->focal / 2 ||
            center[2] / 4 + 1 >= 4096) continue;
        BfmBedroomView plane = {.focal = view->focal, .translation = {0, 0, center[2]},
            .axes = {{grain->scale,0,0}, {0,grain->scale,0}, {0,0,4096}}};
        int16_t offset[3] = {dust->material.left, dust->material.top, 0};
        int16_t size[3] = {dust->material.width, dust->material.height, 0};
        int anchor[3], extent[3];
        if (!bfm_bedroom_project(&plane, offset, anchor) || !bfm_bedroom_project(&plane, size, extent))
            continue;
        int x = center[0] + anchor[0] - 160, y = center[1] + anchor[1] - 120;
        BfmForestDustCard *card = &cards[count++];
        *card = (BfmForestDustCard){.slot = 126 + draw, .bucket = (unsigned)center[2] / 4 + 1,
            .material = {.page = frame->page, .clut = frame->palette, .count = 4,
                         .textured = 1, .translucent = 1}};
        for (unsigned i = 0; i < 4; ++i)
            card->vertices[i] = (BfmForestRasterVertex){
                .x = x + (i & 1 ? extent[0] - 160 : 0), .y = y + (i & 2 ? extent[1] - 120 : 0),
                .u = frame->uv[i][0], .v = frame->uv[i][1], .rgb = 0x808080};
    }
    return !count || bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}

/* Kind 27 follows the scroll from release to pickup. On its constructor
 * update the quad is still centered on the owner. Thereafter the level
 * summit floor supplies its center and reverses both texture axes.
 * shadow_ink is neutral RGB (128,128,128) shaded by the active room lamps
 * for the world normal (0,4096,0), packed with red in the low byte. */
static int earth_scroll_shadow_draw(const uint8_t *scene, size_t bytes,
    BfmBedroomFigure *figure, const BfmBedroomView *view, const int16_t owner_xyz[3],
    int at_birth, uint32_t shadow_ink)
{
    const size_t descriptor = UINT32_C(0x80192648) - UINT32_C(0x80128158);
    if (!scene || !figure || !view || !owner_xyz || bytes < descriptor + 12 ||
        view->focal <= 0 || view->focal > INT16_MAX || (at_birth != 0 && at_birth != 1))
        return 0;
    const uint8_t *sprite = scene + descriptor;
    int width = sprite[2], height = sprite[3];
    int left = s16le(sprite + 8), top = s16le(sprite + 10);
    if (u16le(sprite) != 255 || u16le(sprite + 4) || u16le(sprite + 6) ||
        !width || width > 256 - 64 || !height || height > 256 - 160 ||
        left + width > INT16_MAX || top + height > INT16_MAX)
        return 0;

    BfmBedroomRoot support = {.xyz = {owner_xyz[0], owner_xyz[1], owner_xyz[2]}};
    const int16_t *center = owner_xyz;
    int scale = 2;
    if (!at_birth) {
        if (!bfm_bedroom_travel_air_shadow(scene, bytes, UINT32_C(0x801961e8), &support))
            return 0;
        if (!support.shadow_visible) return 1;
        if (support.shadow_normal[0] || support.shadow_normal[2] || support.shadow_normal[1] >= 0)
            return 0;
        center = support.shadow_xyz;
        scale = -2;
    }

    /* Compose the world anchor into camera space before projecting the four
     * local corners. The original rounds the anchor once at this step. */
    BfmBedroomView sprite_view = *view;
    for (unsigned row = 0; row < 3; ++row) {
        int64_t offset = 0;
        for (unsigned axis = 0; axis < 3; ++axis)
            offset += (int64_t)view->axes[row][axis] * center[axis];
        sprite_view.translation[row] += (int)(offset / 4096 - (offset % 4096 < 0));
        sprite_view.axes[row][0] = scale * view->axes[row][0];
        sprite_view.axes[row][1] = scale * view->axes[row][2];
    }
    BfmForestDustCard card = {
        .material = {.count = 4, .textured = 1, .translucent = 1, .page = 0x59, .clut = 0x4516}
    };
    unsigned depth_sum = 0;
    for (unsigned i = 0; i < 4; ++i) {
        int right = i & 1, bottom = (i >> 1) & 1;
        int16_t corner[3] = {(int16_t)(left + right * width), (int16_t)(top + bottom * height), 0};
        int screen[3];
        if (!bfm_bedroom_project(&sprite_view, corner, screen) || screen[2] <= view->focal / 2 ||
            screen[2] > INT16_MAX || screen[0] < -1024 || screen[0] > 1023 ||
            screen[1] < -1024 || screen[1] > 1023)
            return 1;
        card.vertices[i] = (BfmForestRasterVertex){
            .x = screen[0], .y = screen[1], .u = (uint8_t)(64 + right * (width - 1)),
            .v = (uint8_t)(160 + bottom * (height - 1)), .rgb = shadow_ink & UINT32_C(0xffffff)
        };
        depth_sum += (unsigned)screen[2];
    }
    card.bucket = depth_sum / 16 + 1;
    return card.bucket >= 4096 || bfm_bedroom_figure_add_fusion(figure, view, NULL, &card, 1);
}

enum { ECHOES = 3 };
typedef struct {
    int16_t xyz[3];
    unsigned field, duration;
    int born;
} ScrollEcho;

struct BfmWoodsEarthShrineSceneArt {
    BfmWoodsEarthShrineView *view;
    BfmWoodsEarthShrineFragments *fragments;
    BfmWoodsEarthShrineBurst burst;
    ScrollEcho echoes[ECHOES];
    EarthScrollDust dust;
    BfmForestTransform completed_model;
    const uint8_t *scene;
    size_t scene_bytes;
    unsigned main_clock;
    uint32_t shadow_ink;
    int has_model, shattered, burst_on_screen, shadow_at_birth;
    unsigned glow_stage, pulse;
    int glow_x, glow_y, red, green, blue, new_light;
};

BfmWoodsEarthShrineSceneArt *bfm_woods_earth_shrine_scene_art_open(
    const uint8_t *scene, size_t bytes)
{
    BfmWoodsEarthShrineSceneArt *art = calloc(1, sizeof *art);
    if (!art) return NULL;
    art->scene = scene;
    art->scene_bytes = bytes;
    art->view = bfm_woods_earth_shrine_view_open(scene, bytes);
    art->fragments = bfm_woods_earth_shrine_fragments_open(scene, bytes);
    if (!art->view || !art->fragments) {
        bfm_woods_earth_shrine_scene_art_free(art);
        return NULL;
    }
    return art;
}

void bfm_woods_earth_shrine_scene_art_free(BfmWoodsEarthShrineSceneArt *art)
{
    if (!art) return;
    bfm_woods_earth_shrine_view_free(art->view);
    bfm_woods_earth_shrine_fragments_free(art->fragments);
    free(art);
}

int bfm_woods_earth_shrine_scene_art_bind_smoke(BfmWoodsEarthShrineSceneArt *art,
    const uint8_t *main, size_t bytes)
{
    return art && earth_scroll_dust_load(&art->dust, main, bytes);
}

void bfm_woods_earth_shrine_scene_art_set_field(BfmWoodsEarthShrineSceneArt *art,
    unsigned main_clock, const uint8_t third_lamp_rgb[3])
{
    if (!art || !third_lamp_rgb) return;
    art->main_clock = main_clock;
    /* Only the third directional light contributes to the shadow sprite's
     * upward material normal in this room. Ambient is half intensity. */
    art->shadow_ink = 0;
    for (unsigned channel = 0; channel < 3; ++channel) {
        unsigned light = 2048u + third_lamp_rgb[channel] * 4096u / 255u;
        art->shadow_ink |= (128u * light / 4096u) << (channel * 8);
    }
}

static unsigned release_random(void *context)
{
    return bfm_gameplay_random(context);
}

static int shatter(BfmWoodsEarthShrineSceneArt *art, const BfmEarthShrineRelease *child,
                   BfmGameplayRun *run, const BfmBedroomView *view)
{
    if (art->shattered) return 1;
    if (!run || !view || child->stage != BFM_EARTH_RISING) return 0;
    /* The parent stays at ground level. Detachment has already lifted the
     * child 160 and moved it 16 units forward before issuing this event. */
    int16_t parent[3] = {child->xyz[0], child->original_y, (int16_t)(child->xyz[2] + 16)};
    if (!bfm_woods_earth_shrine_fragments_release(art->fragments, parent, 0, run)) return 0;
    int center[3];
    art->burst_on_screen = bfm_bedroom_project(view, child->xyz, center);
    if (!bfm_woods_earth_shrine_burst_start(&art->burst, center, release_random, run)) return 0;
    art->shadow_at_birth = 2;
    art->shattered = 1;
    return 1;
}

static int add_echo(BfmWoodsEarthShrineSceneArt *art, const BfmEarthShrineRelease *child)
{
    if (child->stage != BFM_EARTH_UNROLLING || !child->animation_duration) return 0;
    for (unsigned i = 0; i < ECHOES; ++i) {
        ScrollEcho *echo = &art->echoes[i];
        if (echo->duration) continue;
        *echo = (ScrollEcho){.duration = child->animation_duration, .born = 1};
        memcpy(echo->xyz, child->xyz, sizeof echo->xyz);
        return 1;
    }
    return 0;
}

int bfm_woods_earth_shrine_scene_art_notify(BfmWoodsEarthShrineSceneArt *art,
    const BfmEarthShrineRelease *child, BfmEarthReleaseEvent event, unsigned kind,
    BfmGameplayRun *run, const BfmBedroomView *view)
{
    if (!art || !child) return 0;
    switch (event) {
    case BFM_EARTH_CREST_STAGE:
        if (kind > 5) return 0;
        art->glow_stage = kind;
        if (kind == 1) {
            art->glow_x = 2048;
            art->glow_y = 4096;
            art->red = art->green = art->blue = 0;
            art->new_light = 1;
        }
        if (kind == 5) art->pulse = 0;
        return 1;
    case BFM_EARTH_DUST:
        return earth_scroll_dust_emit(&art->dust, child->xyz, art->main_clock) != 0;
    case BFM_EARTH_SHATTER:
        return shatter(art, child, run, view);
    case BFM_EARTH_TRAIL:
        return add_echo(art, child);
    default:
        return 0;
    }
}

static void step_light(BfmWoodsEarthShrineSceneArt *art)
{
    if (art->new_light) {
        art->new_light = 0;
        return;
    }
    switch (art->glow_stage) {
    case 1:
        if (art->red < 192) art->red += 4;
        if (art->green < 192) art->green += 4;
        if (art->blue < 32) art->blue += 4;
        break;
    case 2: case 3:
        if (art->glow_stage == 3) {
            if (art->glow_x < 24576) art->glow_x += 2048;
            if (art->glow_y < 24576) art->glow_y += 2048;
        }
        if (art->red < 248) {
            art->red += 4;
            art->green += 4;
            art->blue += 8;
        }
        break;
    case 4: case 5:
        if (art->glow_x > 2048) art->glow_x -= 512;
        if (art->glow_y > 2048) art->glow_y -= 512;
        if (art->glow_stage == 5) {
            ++art->pulse;
            int half = (int)(art->pulse & 15u) / 2;
            int level = art->pulse & 16u ? 9 + half : 16 - half;
            art->red = art->green = 12 * level;
            art->blue = 2 * level;
        } else if (art->red > 192) {
            art->red -= 4;
            art->green -= 4;
            art->blue -= 8;
        }
        break;
    }
}

void bfm_woods_earth_shrine_scene_art_update(BfmWoodsEarthShrineSceneArt *art,
    const BfmEarthShrineRelease *child)
{
    if (!art || !child) return;
    step_light(art);
    earth_scroll_dust_step(&art->dust);
    if (art->shadow_at_birth) --art->shadow_at_birth;
    bfm_woods_earth_shrine_fragments_step(art->fragments, child->original_y);
    bfm_woods_earth_shrine_burst_step(&art->burst);
    for (unsigned i = 0; i < ECHOES; ++i) {
        ScrollEcho *echo = &art->echoes[i];
        if (!echo->duration) continue;
        echo->xyz[1] -= 2;
        echo->xyz[2] -= 3;
        if (!echo->born && ++echo->field >= echo->duration) echo->duration = 0;
        echo->born = 0;
    }
    /* Pickup reads the completed world model from the preceding actor pass.
     * The later screen-facing sprite projection has no bearing on its edges. */
    art->has_model = child->stage == BFM_EARTH_FLOATING;
    if (art->has_model) {
        art->completed_model = (BfmForestTransform){
            .rotation = {{child->scale[0],0,0},{0,child->scale[1],0},{0,0,4096}},
            .translation = {child->xyz[0], child->xyz[1], child->xyz[2]}
        };
    }
}

const BfmForestTransform *bfm_woods_earth_shrine_scene_art_scroll_model(
    const BfmWoodsEarthShrineSceneArt *art)
{
    return art && art->has_model ? &art->completed_model : NULL;
}

int bfm_woods_earth_shrine_scene_art_draw(const BfmWoodsEarthShrineSceneArt *art,
    const BfmEarthShrineRelease *child, unsigned actor_age, unsigned texture_member,
    BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!art || !child || !figure || !view) return 0;
    if (child->stage == BFM_EARTH_SEALED)
        return bfm_woods_earth_shrine_view_draw(art->view, figure, view, 1, actor_age);
    if (child->stage == BFM_EARTH_SHAKING) {
        /* Contact stops the engraving's animation. Only its two stone models
         * follow the horizontal displacement on the blade-hit field. */
        unsigned crest_age = actor_age >= child->shake_ticks ? actor_age - child->shake_ticks : 0;
        if (!bfm_woods_earth_shrine_view_attached(art->view, figure, view,
                                                 child->tablet_dx, crest_age)) return 0;
    } else if (child->stage >= BFM_EARTH_RISING && child->stage <= BFM_EARTH_TAKEN &&
               child->scale[0] > 0) {
        unsigned size = (unsigned)child->scale[0] / 1408;
        BfmWoodsEarthScrollForm form = child->stage <= BFM_EARTH_HOVERING ?
            BFM_EARTH_SCROLL_PANEL : BFM_EARTH_SCROLL_UNFOLDING;
        if (!bfm_woods_earth_shrine_view_scroll(art->view, figure, view, texture_member,
                                                form, child->xyz, child->animation_age, size)) return 0;
    }
    for (unsigned i = 0; i < ECHOES; ++i) {
        const ScrollEcho *echo = &art->echoes[i];
        if (echo->duration && !bfm_woods_earth_shrine_view_scroll(art->view, figure, view,
            texture_member, BFM_EARTH_SCROLL_ECHO, echo->xyz, echo->field, 8)) return 0;
    }
    if (art->glow_stage) {
        uint32_t rgb = (unsigned)art->red | (unsigned)art->green << 8 | (unsigned)art->blue << 16;
        if (rgb && !bfm_woods_earth_shrine_view_glow(figure, view, child->xyz,
            child->stage == BFM_EARTH_SHAKING, art->glow_x, art->glow_y, rgb)) return 0;
    }
    if (art->shattered && child->stage >= BFM_EARTH_RISING && child->stage <= BFM_EARTH_FLOATING &&
        !earth_scroll_shadow_draw(art->scene, art->scene_bytes, figure, view, child->xyz,
                                  art->shadow_at_birth != 0, art->shadow_ink)) return 0;
    if (!earth_scroll_dust_draw(&art->dust, figure, view) ||
        !bfm_woods_earth_shrine_fragments_draw(art->fragments, figure, view)) return 0;
    return !art->burst_on_screen || bfm_woods_earth_shrine_burst_append(&art->burst, figure, view);
}
