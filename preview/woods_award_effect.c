/* SPDX-License-Identifier: MIT */
#include "woods_award_effect.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum { SECTOR = 2048, TILE = 32, PICTURE_TILES = 3, UNIT = 4096 };
enum { INACTIVE, CONTRACTING, HOLDING, BRIGHTENING, FADING };

typedef struct {
    unsigned member, palette_sector, palette_command, picture_sector;
} AwardPicture;

static const AwardPicture satiate = {18, 4, 0x838, 0};
static const AwardPicture shrink = {19, 3, 0x1358, 19};

/* The three vertically adjacent 32-word tiles are in a separate upload from
 * the skill's equipment art. Validate the entire upload before touching VRAM. */
static int upload_picture(const MusashiDiscMedia *disc, const AwardPicture *picture,
                          unsigned word_x, unsigned y,
                          uint16_t tiles[512][1024], uint16_t *clut)
{
    uint8_t header[SECTOR], image[PICTURE_TILES * SECTOR], command[20], palette[32];
    size_t total;
    if (!disc || !tiles || word_x % TILE || y % TILE || word_x > 1024 - TILE ||
        y > 512 - PICTURE_TILES * TILE ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, header, sizeof header, &total))
        return 0;
    uint32_t count = u32le(header);
    if (count <= picture->member || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = header + 8 + 8 * picture->member;
    uint32_t start = u32le(entry), length = u32le(entry + 4);
    if (!start || start > total / SECTOR || length > total - (size_t)start * SECTOR ||
        length < (picture->picture_sector + 1 + PICTURE_TILES) * SECTOR ||
        length < (picture->palette_sector + 1) * SECTOR) return 0;
    size_t member = (size_t)start * SECTOR;
    size_t section = member + picture->palette_sector * SECTOR;
    const size_t command_offset = picture->palette_command;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", section, header, sizeof header, &total) ||
        memcmp(header, "PAC", 4) || u32le(header + 4) != 1) return 0;
    size_t section_bytes = u32le(header + 12);
    if (section_bytes > length - picture->palette_sector * SECTOR ||
        section_bytes < SECTOR + command_offset + sizeof command) return 0;
    size_t payload = section + SECTOR;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", payload + command_offset, command,
                                            sizeof command, &total) ||
        u32le(command) != (44u << 16 | 9u) || u16le(command+8) != 16 ||
        u16le(command+10) != 1 || u32le(command+16) != 255) return 0;
    unsigned cx = u16le(command+4), cy = u16le(command+6);
    uint32_t colors = u32le(command+12), origin = UINT32_C(0x800cae08);
    if (cx % 16 || cx > 1024-16 || cy >= 512 || colors < origin ||
        colors-origin > section_bytes - SECTOR - sizeof palette ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", payload + (colors-origin), palette,
                                            sizeof palette, &total)) return 0;

    size_t at = member + picture->picture_sector * SECTOR;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", at, header, sizeof header, &total) ||
        memcmp(header, "PAC", 4) || u32le(header + 4) || u32le(header + 8) != PICTURE_TILES ||
        u32le(header + 12) != sizeof image + SECTOR) return 0;
    for (unsigned row = 0; row < 512 / TILE; ++row) {
        int covered = row >= y / TILE && row < y / TILE + PICTURE_TILES;
        uint32_t expected = covered ? UINT32_C(1) << (word_x / TILE) : 0;
        if (u32le(header + 16 + 4 * row) != expected) return 0;
    }
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", at + SECTOR, image, sizeof image, &total))
        return 0;
    for (unsigned i = 0; i < 16; ++i) tiles[cy][cx+i] = u16le(palette + 2*i);
    *clut = (uint16_t)(cy * 64 + cx / 16);
    for (unsigned row = 0; row < PICTURE_TILES * TILE; ++row)
        for (unsigned col = 0; col < TILE; ++col)
            tiles[y + row][word_x + col] = u16le(image + 2 * (row * TILE + col));
    return 1;
}

/* The room supplies local screen corners and absolute UVs. Its effect turns
 * the landscape-format mounted artwork a quarter turn onto this portrait. */
static int load_shape(BfmWoodsAwardEffect *effect, const uint8_t *scene, size_t size,
                      unsigned *word_x, unsigned *row_y)
{
    const size_t offset = UINT32_C(0x8018a444) - UINT32_C(0x80128158);
    if (!scene || size < offset + 48) return 0;
    const uint8_t *corners = scene + offset, *uv = corners + 32;
    unsigned x = u16le(uv), y = u16le(uv + 2);
    if ((x & 127) || x > 4096 - 128 || (y & 31) || y > 512 - 96 ||
        (y % 256) + 96 > 256) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        effect->picture.vertices[i].x = s16le(corners + 8*i);
        effect->picture.vertices[i].y = s16le(corners + 8*i + 2);
        if (s16le(corners + 8*i + 4) || u16le(uv + 4*i) != x + (i & 1)*127 ||
            u16le(uv + 4*i + 2) != y + (i >> 1)*95) return 0;
        unsigned j = ((i & 1) ? 0 : 2) + (i >> 1);
        effect->picture.vertices[i].u = (uint8_t)u16le(uv + 4*j);
        effect->picture.vertices[i].v = (uint8_t)u16le(uv + 4*j + 2);
    }
    effect->picture.bucket = 3;
    effect->picture.material = (BfmForestRasterMaterial){
        .count = 4, .textured = 1, .translucent = 1,
        .page = (uint16_t)(x / 256 | (y / 256) << 4 | 1u << 5)
    };
    *word_x = x / 4;
    *row_y = y;
    return 1;
}

static void contract(BfmWoodsAwardEffect *effect)
{
    effect->speed += 64;
    effect->scale -= effect->speed;
    effect->light += 4;
    if (effect->scale <= UNIT) {
        effect->scale = UNIT;
        effect->hold = 14;
        effect->phase = HOLDING;
    }
    unsigned interval = effect->scale >= 0x2800 ? 3 : 2;
    if (++effect->counter != interval) return;
    effect->counter = 0;
    for (unsigned i = 0; i < BFM_WOODS_AWARD_ECHOES; ++i) {
        if (effect->echoes[i].remaining) continue;
        effect->echoes[i] = (BfmWoodsAwardEcho){effect->scale, effect->light, 8};
        break;
    }
}

int bfm_woods_award_effect_step_ability(BfmWoodsAwardEffect *effect, const MusashiDiscMedia *disc,
                                       const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024],
                                       unsigned released_ability)
{
    if (!effect) return 0;
    if (released_ability) {
        const AwardPicture *picture;
        switch (released_ability) {
        case 3: picture = &satiate; break;
        case 13: picture = &shrink; break;
        default: return 0;
        }
        BfmWoodsAwardEffect fresh = {.phase = CONTRACTING, .scale = 4*UNIT};
        unsigned x, y;
        if (effect->phase || !load_shape(&fresh, scene, bytes, &x, &y) ||
            !upload_picture(disc, picture, x, y, tiles, &fresh.picture.material.clut)) return 0;
        *effect = fresh;
        return 1;
    }
    if (!effect->phase) return 1;
    /* Each previous submission consumed one of its eight updates. Count that
     * before allocating the next snapshot, independently of display cadence. */
    for (unsigned i = 0; i < BFM_WOODS_AWARD_ECHOES; ++i)
        if (effect->echoes[i].remaining) --effect->echoes[i].remaining;
    switch (effect->phase) {
    case CONTRACTING:
        contract(effect);
        break;
    case HOLDING:
        if (!--effect->hold) effect->phase = BRIGHTENING;
        break;
    case BRIGHTENING:
        effect->light += 16;
        if (effect->light >= 256) {
            effect->light = 255;
            effect->phase = FADING;
        }
        break;
    case FADING:
        effect->light -= 16;
        if (effect->light < 0) effect->phase = INACTIVE;
        break;
    default: return 0;
    }
    return 1;
}

int bfm_woods_award_effect_step(BfmWoodsAwardEffect *effect, const MusashiDiscMedia *disc,
                               const uint8_t *scene, size_t bytes, uint16_t tiles[512][1024],
                               int release_award)
{
    return bfm_woods_award_effect_step_ability(effect, disc, scene, bytes, tiles,
                                              release_award ? 13 : 0);
}

static int scale_corner(int coordinate, int scale)
{
    int product = coordinate * scale;
    return product / UNIT - (product % UNIT < 0);
}

static BfmForestDustCard scaled_picture(const BfmForestDustCard *picture, int scale, int light)
{
    BfmForestDustCard card = *picture;
    for (unsigned i = 0; i < 4; ++i) {
        card.vertices[i].x = 160 + scale_corner(picture->vertices[i].x, scale);
        card.vertices[i].y = 120 + scale_corner(picture->vertices[i].y, scale);
        card.vertices[i].rgb = (uint32_t)light * UINT32_C(0x010101);
    }
    return card;
}

int bfm_woods_award_effect_append(const BfmWoodsAwardEffect *effect, BfmBedroomFigure *figure,
                                  const BfmBedroomView *view)
{
    if (!effect || !figure || !view) return 0;
    if (!effect->phase || !effect->light) return 1;
    BfmForestDustCard cards[1 + BFM_WOODS_AWARD_ECHOES];
    cards[0] = scaled_picture(&effect->picture, effect->scale, effect->light);
    size_t count = 1;
    for (unsigned i = 0; i < BFM_WOODS_AWARD_ECHOES; ++i) {
        const BfmWoodsAwardEcho *echo = &effect->echoes[i];
        if (echo->remaining)
            cards[count++] = scaled_picture(&effect->picture, echo->scale, echo->light);
    }
    return bfm_bedroom_figure_add_fusion(figure, view, NULL, cards, count);
}
