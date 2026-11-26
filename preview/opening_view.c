/* SPDX-License-Identifier: MIT */
#include "scene_views.h"
#include "chapter_card.h"
#include "forest_walk.h"
#include "scene_payload.h"
#include "scene_tiles.h"
#include "palace.h"
#include "palace_dialogue.h"
#include <stdio.h>
#include <stdlib.h>

enum { WIDTH = 320, HEIGHT = 240, INTERIOR_START_FIELD = 1188, INTERIOR_DRAW_FIELD = 1418 };
enum { TITLE_WIDTH = 256, TITLE_HEIGHT = 64 };
enum { CREDIT_X = 80, CREDIT_Y = 104, CREDIT_WIDTH = 160, CREDIT_HEIGHT = 32 };

typedef enum { FOREST_WAITING, FOREST_QUEUED, FOREST_READY, FOREST_TAKEN } ForestStage;

typedef struct {
    Palace *palace, *stairs;
    BfmChapterCard *chapter;
    unsigned departure_field, departure_pose, departure_member;
    int natural_departure;
    unsigned playtime_updates;
    ForestStage forest_stage;
    RoomTransferDescriptor forest_entry;
    BfmForestWalk *incoming_forest;
    uint16_t (*tiles)[1024], (*stair_tiles)[1024];
    uint16_t sky[480][320];
    uint16_t title_ink[TITLE_HEIGHT][TITLE_WIDTH];
    uint16_t title_mask[TITLE_HEIGHT][TITLE_WIDTH];
    uint16_t lettering[CREDIT_HEIGHT][CREDIT_WIDTH];
    uint16_t silhouette[CREDIT_HEIGHT][CREDIT_WIDTH];
} OpeningView;

/* The opening paints the sky with screen-aligned textured quads. Nearest
 * texel interpolation at a half-texel lands on opposite sides of the tie
 * in their two triangles when the texture step is fractional in 16.16. */
static unsigned sample_axis(unsigned coordinate, unsigned extent,
                            unsigned last_texel, int first_triangle)
{
    return (2 * coordinate * last_texel + extent - (first_triangle && (last_texel * 65536u) % extent ? 1u : 0u))
           / (2 * extent);
}

static uint16_t read_sky(const uint16_t words[512][1024], unsigned x, unsigned y)
{
    unsigned page_x = x < 256 ? (y < 256 ? 896 : 768) : 960;
    unsigned page_y = x < 256 ? 0 : 256;
    unsigned u = x < 256 ? x : x - 256 + (y < 256 ? 64 : 0);
    unsigned v = y < 256 ? y : y - 256;
    uint16_t pair = words[page_y + v][page_x + u / 2];
    return words[503][(pair >> (8 * (u & 1))) & 255u] & 0x7fffu;
}

typedef struct {
    int left, right, seam, top, middle, bottom;
} SkyProjection;

static SkyProjection projection(unsigned fields)
{
    unsigned steps = fields > 248 ? (fields - 248) / 2 : 0;
    int scale = steps < 23 ? 5880 - 80 * (int)steps : 4096;
    /* After the game title fades, the camera tilts down by three world
     * pixels every two 30 Hz animation ticks. */
    unsigned pan_ticks = fields > 508 ? (fields - 508) / 2 : 0;
    int pan = (int)((3 * pan_ticks + 1) / 2);
    if (pan > 239) pan = 239;
    return (SkyProjection){
        160 - 160 * scale / 4096, 160 + 160 * scale / 4096,
        160 + 96 * scale / 4096, 120 - 120 * scale / 4096 - pan,
        120 + 136 * scale / 4096 - pan, 120 + 360 * scale / 4096 - pan
    };
}

static uint16_t sky_texel(const OpeningView *view, const SkyProjection *p, int x, int y)
{
    int left = x < p->seam;
    int top = y < p->middle;
    unsigned local_x = (unsigned)(x - (left ? p->left : p->seam));
    unsigned local_y = (unsigned)(y - (top ? p->top : p->middle));
    unsigned w = (unsigned)(left ? p->seam - p->left : p->right - p->seam);
    unsigned h = (unsigned)(top ? p->middle - p->top : p->bottom - p->middle);
    int first_triangle = local_x * h + local_y * w < w * h;
    unsigned u = (left ? 0u : 256u) +
                 sample_axis(local_x, w, left ? 255u : 63u, first_triangle);
    unsigned v = (top ? 0u : 256u) +
                 sample_axis(local_y, h, top ? 255u : 223u, first_triangle);
    return view->sky[v][u];
}

int bfm_opening_load(const MusashiDiscMedia *disc, void **scene,
                     char *error, size_t error_size)
{
    if (!scene)
        return 0;
    *scene = NULL;
    uint16_t (*words)[1024] = calloc(512, sizeof(*words));
    OpeningView *view = calloc(1, sizeof(*view));
    if (!words || !view || !bfm_scene_read_tiles(disc, "SC01.CD", 0, words)) {
        free(words);
        free(view);
        if (error && error_size)
            snprintf(error, error_size, "Could not load the opening artwork from SC01.CD.");
        return 0;
    }
    for (unsigned y = 0; y < 480; ++y)
        for (unsigned x = 0; x < WIDTH; ++x)
            view->sky[y][x] = read_sky(words, x, y);
    for (unsigned y = 0; y < CREDIT_HEIGHT; ++y) {
        for (unsigned x = 0; x < CREDIT_WIDTH; ++x) {
            unsigned shift = 4 * (x % 4);
            unsigned page_x = 896 + x / 4;
            unsigned letter = (words[256 + y][page_x] >> shift) & 15;
            unsigned mask = (words[288 + y][page_x] >> shift) & 15;
            view->lettering[y][x] = words[256][368 + letter];
            view->silhouette[y][x] = words[257][368 + mask];
        }
    }
    for (unsigned y = 0; y < TITLE_HEIGHT; ++y) {
        for (unsigned x = 0; x < TITLE_WIDTH; ++x) {
            unsigned shift = 4 * (x % 4);
            unsigned page_x = 896 + x / 4;
            unsigned ink = (words[320 + y][page_x] >> shift) & 15;
            unsigned mask = (words[400 + y][page_x] >> shift) & 15;
            view->title_ink[y][x] = words[258][368 + ink];
            view->title_mask[y][x] = words[260][368 + mask];
        }
    }
    view->palace = bfm_palace_load(disc, 0, words);
    view->stair_tiles = calloc(512, sizeof(*view->stair_tiles));
    if (view->stair_tiles && bfm_scene_read_tiles(disc, "MAIN.CD", 10, view->stair_tiles) &&
        bfm_scene_read_tiles(disc, "SC01.CD", 1, view->stair_tiles))
        view->stairs = bfm_palace_load(disc, 1, view->stair_tiles);
    if (!view->palace || !view->stairs) {
        if (error && error_size)
            snprintf(error, error_size, "Could not read the opening palace from SC01.CD.");
        bfm_palace_free(view->stairs);
        bfm_palace_free(view->palace);
        free(view->stair_tiles);
        free(words);
        free(view);
        return 0;
    }
    view->tiles = words;
    *scene = view;
    return 1;
}

static uint32_t faded_channel(unsigned value, unsigned subtraction)
{
    return value > subtraction ? (value - subtraction) * 255 / 31 : 0;
}

static unsigned credit_strength(unsigned fields)
{
    if (fields < 124 || fields >= 246)
        return 0;
    unsigned rise = (fields - 124) / 2;
    unsigned fall = (246 - (fields & ~1u)) / 2;
    unsigned steps = rise < fall ? rise : fall;
    return (steps < 16 ? steps : 16) * 8;
}

static uint16_t blend_credit(uint16_t backdrop, uint16_t silhouette, uint16_t letter,
                             unsigned strength)
{
    uint16_t result = 0;
    for (unsigned shift = 0; shift <= 10; shift += 5) {
        unsigned back = (backdrop >> shift) & 31;
        unsigned loss = ((silhouette >> shift) & 31) * strength / 128;
        unsigned ink = ((letter >> shift) & 31) * strength / 128;
        unsigned value = (back > loss ? back - loss : 0) + ink;
        result |= (uint16_t)((value < 32 ? value : 31) << shift);
    }
    return result;
}

/* Each line of the game title opens from its middle. The top and bottom
 * strips grow by one source row per animation tick and then fade together. */
static uint16_t title_line(const OpeningView *view, uint16_t back, int x, int y,
                           int center, int first_row, int rows, unsigned level)
{
    int dy = y - center;
    if (dy < -rows || dy > rows)
        return back;
    int row = first_row + (dy < 0 ? dy + rows : 31 - rows + dy);
    return blend_credit(back, view->title_mask[row][x],
                        view->title_ink[row][x], level);
}

static uint16_t game_title(const OpeningView *view, uint16_t back, unsigned fields,
                           int x, int y)
{
    if (fields >= 324 && fields < 506 && x >= 32 && x < 288 &&
        y >= 88 && y <= 152) {
        int rows = (int)((fields - 324) / 2);
        if (rows > 16) rows = 16;
        unsigned level = 8 * ((506 - (fields & ~1u)) / 2);
        if (level > 128) level = 128;
        back = title_line(view, back, x - 32, y, 136, 32, rows, level);
        back = title_line(view, back, x - 32, y, 104, 0, rows, level);
    }
    if (fields >= 294 && fields < 356 && (y == 104 || y == 136)) {
        int tick = 1 + (int)((fields - 294) / 2);
        int length = tick * (26 + 6 * tick);
        if (length > 256) length = 256;
        int left = y == 104 ? 32 : 288 - length;
        if (x >= left && x <= left + length) {
            int start = 16 * (tick + (tick >= 6));
            if (start > 255) start = 255;
            int color = start + (x - left) * (255 - start) / length;
            back = (uint16_t)((color >> 3) << (y == 104 ? 0 : 10));
        }
    }
    return back;
}

/* Scene change fades subtract a shade from each five-bit color channel. */
static void shade_scene(uint32_t *pixels, unsigned shade)
{
    if (!shade) return;
    for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {
        uint32_t rgb = pixels[i];
        uint32_t result = UINT32_C(0xff000000);
        for (unsigned shift = 0; shift <= 16; shift += 8) {
            unsigned value = (((rgb >> shift) & 255u) * 31 + 127) / 255;
            result |= faded_channel(value, shade) << shift;
        }
        pixels[i] = result;
    }
}

/* Both flashes are drawn last on the active display page. After the second
 * white hold, the additive gray falls by four on each 30 Hz tick. The light
 * that fills the screen immediately before it is part of the world geometry. */
static void flash_ritual(uint32_t *pixels, unsigned field)
{
    unsigned gray;
    if (field >= 4563 && field < 4569) {
        gray = 255;
    } else if (field >= 4632 && field < 4762) {
        unsigned tick = (field - 4632) / 2;
        gray = tick < 2 ? 255 : 255 - 4 * (tick - 1);
    } else {
        return;
    }
    unsigned light = gray >> 3;
    if (!light) return;
    for (unsigned i = WIDTH * 10; i < WIDTH * (HEIGHT - 10); ++i) {
        uint32_t out = UINT32_C(0xff000000);
        for (unsigned shift = 0; shift <= 16; shift += 8) {
            unsigned channel = ((pixels[i] >> shift) & 255u) >> 3;
            channel += light;
            if (channel > 31) channel = 31;
            out |= channel * 255 / 31 << shift;
        }
        pixels[i] = out;
    }
}

static int draw_opening(const OpeningView *view, unsigned fields, uint32_t *pixels)
{
    if (fields >= INTERIOR_START_FIELD) {
        for (unsigned i = 0; i < WIDTH * HEIGHT; ++i)
            pixels[i] = UINT32_C(0xff000000);
        if (fields >= INTERIOR_DRAW_FIELD) {
            bfm_palace_draw(view->stairs, view->stair_tiles, fields, pixels);
            unsigned tick = (fields - INTERIOR_DRAW_FIELD) / 2;
            shade_scene(pixels, tick < 16 ? 31 - 2 * tick : 0);
            flash_ritual(pixels, fields);
        }
        return 1;
    }

    /* The original subtracts the same shade on each RGB555 channel and
     * advances it every other display field, then holds the moon fully lit. */
    unsigned darkness = fields < 34 ? 33 - 2 * (fields / 2) : 0;
    SkyProjection camera = projection(fields);
    unsigned strength = credit_strength(fields);
    for (unsigned y = 0; y < HEIGHT; ++y) {
        for (unsigned x = 0; x < WIDTH; ++x) {
            uint16_t word = y >= 10 && y < HEIGHT - 10
                                ? sky_texel(view, &camera, (int)x, (int)y) : 0;
            if (strength && y >= CREDIT_Y && y < CREDIT_Y + CREDIT_HEIGHT &&
                x >= CREDIT_X && x < CREDIT_X + CREDIT_WIDTH)
                word = blend_credit(word, view->silhouette[y - CREDIT_Y][x - CREDIT_X],
                                    view->lettering[y - CREDIT_Y][x - CREDIT_X], strength);
            if (fields >= 294 && fields < 506)
                word = game_title(view, word, fields, (int)x, (int)y);
            uint32_t r = faded_channel(word & 31, darkness);
            uint32_t g = faded_channel((word >> 5) & 31, darkness);
            uint32_t b = faded_channel((word >> 10) & 31, darkness);
            pixels[y * WIDTH + x] = UINT32_C(0xff000000) | (r << 16) | (g << 8) | b;
        }
    }
    bfm_palace_draw(view->palace, view->tiles, fields, pixels);
    if (fields >= 1064)
        shade_scene(pixels, (fields - 1064) / 4);
    return 1;
}

static int begin_chapter(OpeningView *view, const MusashiDiscMedia *disc, unsigned field,
                         unsigned pose, unsigned member, char *error, size_t error_size)
{
    BfmChapterCard *card = bfm_chapter_card_load(disc);
    if (!card) {
        if (error && error_size)
            snprintf(error, error_size, "Could not read the Chapter 1 card from SC01.CD.");
        return -1;
    }
    view->chapter = card;
    view->departure_member = member;
    view->departure_field = field;
    view->departure_pose = pose;
    return 1;
}

/* A field number belongs to the opening presentation. Repainting the same
 * field or waiting on a transfer must leave the earned time unchanged. */
void bfm_opening_advance_playtime(void *scene, unsigned field, BfmGameplayRun *run)
{
    enum { EXTERIOR_LIVE = 30, EXTERIOR_LEAVE = 1034, COURT_LIVE = 1445 };
    OpeningView *view = scene;
    if (!view || !run || !run->has_player) return;
    if (field > BFM_OPENING_LAST_FIELD) field = BFM_OPENING_LAST_FIELD;
    if (view->chapter) {
        /* The ordinary court queues its door three updates after the final
         * page closes. The interior Cross is dispatched on its next update. */
        unsigned last = view->departure_pose;
        if (view->natural_departure) last += 5;
        else last += view->departure_member == 1 ? 4u : 2u;
        if (field > last) field = last;
    }
    if (field < EXTERIOR_LIVE) return;
    unsigned exterior = field < EXTERIOR_LEAVE ? field : EXTERIOR_LEAVE;
    unsigned updates = (exterior - EXTERIOR_LIVE) / 2;
    if (field >= COURT_LIVE) updates += (field - COURT_LIVE) / 2;
    if (updates > view->playtime_updates) {
        bfm_gameplay_advance_playtime(run->player.record, updates - view->playtime_updates);
        view->playtime_updates = updates;
    }
}

int bfm_opening_request_skip(void *scene, const MusashiDiscMedia *disc, unsigned fields,
                             char *error, size_t error_size)
{
    OpeningView *view = scene;
    if (!view || !disc) return -1;
    if (view->chapter) return 0;
    unsigned pose = fields < BFM_OPENING_LAST_FIELD ? fields : BFM_OPENING_LAST_FIELD;
    return begin_chapter(view, disc, fields, pose, fields < INTERIOR_START_FIELD ? 0u : 1u,
                         error, error_size);
}

static int read_forest_entry(const MusashiDiscMedia *disc, unsigned member,
                             RoomTransferDescriptor *entry)
{
    size_t size;
    uint8_t *scene = bfm_scene_unpack_pac0(disc, "SC01.CD", member, 1u << 20, 1u << 20, &size);
    if (!scene) return 0;
    int selected = bfm_forest_entry_from_scene(scene, size, member, entry);
    free(scene);
    return selected;
}

static BfmForestWalk *open_departing_forest(const OpeningView *view, const MusashiDiscMedia *disc,
                                          const RoomTransferDescriptor *entry)
{
    BfmForestWalk *walk = bfm_forest_walk_open(disc, entry);
    if (!walk) return NULL;
    bfm_forest_walk_set_palace_exit(walk, view->departure_member);
    unsigned field = view->departure_field;
    if (field != view->departure_pose) return walk;

    /* At the first displayed palace field, two world updates have completed.
     * The exterior and court continue for 29 and 31 updates respectively
     * after a departure request. The chapter card advances neither. */
    unsigned world_tick = 2 + field / 2;
    if (view->departure_member == 0 && field >= 30 && field < INTERIOR_START_FIELD) {
        bfm_forest_walk_inherit_clock(walk, (uint16_t)(world_tick + 29 + 1));
    } else if (view->departure_member == 1 && field >= INTERIOR_DRAW_FIELD) {
        /* Loading the court suspends 113 updates. Count the first forest
         * update as well: its resulting pose is local forest tick zero. */
        bfm_forest_walk_inherit_clock(walk, (uint16_t)(world_tick - 113 + 31 + 1));
    }
    return walk;
}

int bfm_opening_advance_court(void *scene, const MusashiDiscMedia *disc, unsigned field,
                             char *error, size_t error_size)
{
    OpeningView *view = scene;
    if (!view || !disc) return -1;
    /* Shorter opening cuts end before the final camera and its outgoing fade. */
    enum { FULL_COURT_LAST_POSE = 17533 };
    if (BFM_OPENING_LAST_FIELD < FULL_COURT_LAST_POSE) return 0;

    int began = 0;
    if (!view->chapter) {
        unsigned closed;
        if (!bfm_palace_dialogue_departure_field(bfm_palace_speech(view->stairs), &closed) ||
            field < closed) return 0;
        began = begin_chapter(view, disc, closed, closed, 1, error, error_size);
        if (began < 0) return -1;
        view->natural_departure = 1;
    }
    /* The active court queues its second door three scene ticks after the
     * message closes. Keep that record while the outgoing frames and card play. */
    if (!view->natural_departure || field < view->departure_field) return began;
    unsigned elapsed = field - view->departure_field;
    if (view->forest_stage == FOREST_WAITING && elapsed >= 6) {
        if (!read_forest_entry(disc, view->departure_member, &view->forest_entry)) {
            if (error && error_size)
                snprintf(error, error_size, "Could not read the palace's departing forest door.");
            return -1;
        }
        view->forest_stage = FOREST_QUEUED;
    }
    /* The destination is selected while the fading palace is still visible.
     * Its entrance waits at its initial pose until the chapter card ends. */
    if (view->forest_stage == FOREST_QUEUED && elapsed >= 64) {
        view->incoming_forest = open_departing_forest(view, disc, &view->forest_entry);
        if (!view->incoming_forest) {
            if (error && error_size)
                snprintf(error, error_size, "Could not prepare the palace's forest entrance.");
            return -1;
        }
        view->forest_stage = FOREST_READY;
    }
    return began;
}

int bfm_opening_forest_score_field(const void *scene, unsigned field, uint64_t *start_field)
{
    const OpeningView *view = scene;
    if (!view || !view->chapter || !start_field || field < view->departure_field ||
        !bfm_chapter_card_cleared(field - view->departure_field, view->departure_member))
        return 0;
    if (view->natural_departure && view->forest_stage != FOREST_READY) return 0;
    /* The first forest note precedes the room's first update. The exterior
     * departure starts its incoming audio two fields earlier than the court. */
    unsigned lead = view->departure_member ? 6u : 8u;
    *start_field = (uint64_t)view->departure_field + BFM_CHAPTER_FOREST_FIELD - lead;
    return 1;
}

int bfm_opening_skip_member(const void *scene, unsigned *member)
{
    const OpeningView *view = scene;
    if (!view || !view->chapter || view->natural_departure || !member) return 0;
    *member = view->departure_member;
    return 1;
}

BfmForestWalk *bfm_opening_take_forest(void *scene, const MusashiDiscMedia *disc,
                                       unsigned field, uint64_t *forest_field)
{
    OpeningView *view = scene;
    if (!view || !view->chapter || !disc || !forest_field || view->forest_stage == FOREST_TAKEN)
        return NULL;
    uint64_t first = (uint64_t)view->departure_field + BFM_CHAPTER_FOREST_FIELD;
    if (field < first) return NULL;
    BfmForestWalk *forest;
    if (view->natural_departure) {
        if (view->forest_stage != FOREST_READY) return NULL;
        forest = view->incoming_forest;
        view->incoming_forest = NULL;
        *forest_field = first;
    } else {
        RoomTransferDescriptor entry;
        if (!read_forest_entry(disc, view->departure_member, &entry)) return NULL;
        forest = open_departing_forest(view, disc, &entry);
        if (!forest) return NULL;
        *forest_field = 0;
    }
    view->forest_stage = FOREST_TAKEN;
    return forest;
}

int bfm_opening_frame(void *scene, unsigned fields, uint32_t *pixels, int width, int height)
{
    const OpeningView *view = scene;
    if (!view || !pixels || width != WIDTH || height != HEIGHT) return -1;
    if (!view->chapter) {
        if (fields > BFM_OPENING_LAST_FIELD) return BFM_OPENING_END;
        return draw_opening(view, fields, pixels);
    }
    unsigned age = fields >= view->departure_field ? fields - view->departure_field : 0;
    unsigned room_shade;
    BfmChapterFrame stage = bfm_chapter_departure_frame(view->chapter, age, view->departure_member,
                                                       pixels, &room_shade);
    if (stage == BFM_CHAPTER_FINISHED) return BFM_OPENING_FOREST_PENDING;
    if (stage != BFM_CHAPTER_OUTGOING_ROOM) return stage;
    unsigned active = view->departure_pose + age;
    int result = draw_opening(view, active, pixels);
    shade_scene(pixels, room_shade);
    return result;
}

int bfm_opening_voice_schedule(void *scene, unsigned field, unsigned *bank, unsigned *cue,
                              unsigned *issue_field)
{
    OpeningView *view = scene;
    return view && !view->chapter && field <= BFM_OPENING_LAST_FIELD &&
           bfm_palace_dialogue_voice_schedule(bfm_palace_speech(view->stairs), field,
                                            bank, cue, issue_field);
}

int bfm_opening_voice_due(void *scene, unsigned field, unsigned *bank, unsigned *cue)
{
    return bfm_opening_voice_schedule(scene, field, bank, cue, NULL);
}

int bfm_opening_voice_started(void *scene, unsigned bank, unsigned cue, unsigned field)
{
    OpeningView *view = scene;
    return view && !view->chapter && field <= BFM_OPENING_LAST_FIELD &&
           bfm_palace_dialogue_voice_started(bfm_palace_speech(view->stairs), bank, cue, field);
}

int bfm_opening_final_frame(void *scene, uint32_t *pixels, int width, int height)
{
    return bfm_opening_frame(scene, BFM_OPENING_LAST_FIELD, pixels, width, height);
}

void bfm_opening_free(void *scene)
{
    OpeningView *view = scene;
    if (view) {
        bfm_forest_walk_free(view->incoming_forest);
        bfm_chapter_card_free(view->chapter);
        bfm_palace_free(view->palace);
        free(view->tiles);
        bfm_palace_free(view->stairs);
        free(view->stair_tiles);
        free(view);
    }
}
