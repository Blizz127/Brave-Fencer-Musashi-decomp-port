/* SPDX-License-Identifier: MIT */
#include "forest_message_art.h"
#include "dialogue_contour.h"

#include <stdlib.h>

enum { CORNERS = BFM_DIALOGUE_CORNERS, GLYPHS = 0x96 - 0x20, FONT_HEIGHT = 12 };
enum { TAIL_OFFSET = 0x2030, FONT_OFFSET = 0x24cbc, CONTOUR_OFFSET = 0x257fc,
       FOREST_INSET = 0x5f094, BEDROOM_INSET = 0x5a928 };

struct BfmForestMessageArt {
    BfmDialoguePoint contour[2][CORNERS], inset[2][CORNERS];
    int16_t tail_limits[6];
    int has_tail_limits;
    uint8_t glyph[GLYPHS][FONT_HEIGHT];
    uint32_t portrait[4][40][32], tail[16][16], prompt[12][12], ink[16];
    uint32_t paper, highlight, outline;
};

static unsigned le16(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static uint32_t le32(const uint8_t *p)
{
    return le16(p) | (uint32_t)le16(p + 2) << 16;
}

static int signed16(const uint8_t *p)
{
    unsigned n = le16(p);
    return n < 32768 ? (int)n : (int)n - 65536;
}

static int signed8(uint8_t n)
{
    return n < 128 ? n : (int)n - 256;
}

static uint32_t rgb(uint16_t c)
{
    return UINT32_C(0xff000000) | ((c & 31u) * 255 / 31) << 16 |
           (((c >> 5) & 31u) * 255 / 31) << 8 | ((c >> 10) & 31u) * 255 / 31;
}

static int read_art(BfmForestMessageArt *art, const MusashiDiscMedia *disc,
                    const uint8_t *scene, const uint16_t tiles[512][1024])
{
    uint8_t entry[8], contours[2][CORNERS][4], tail[15 * 64 + 8];
    size_t length;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", 8 + 8 * 10, entry,
                                           sizeof(entry), &length))
        return 0;
    uint32_t sector = le32(entry), extent = le32(entry + 4);
    if (!sector || sector > length / 2048)
        return 0;
    size_t base = (size_t)sector * 2048;
    if (extent > length - base || extent < CONTOUR_OFFSET + sizeof(contours) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + CONTOUR_OFFSET,
                                            &contours[0][0][0], sizeof(contours), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + FONT_OFFSET,
                                            &art->glyph[0][0], sizeof(art->glyph), &length) ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", base + TAIL_OFFSET,
                                            tail, sizeof(tail), &length))
        return 0;
    for (unsigned style = 0; style < 2; ++style)
        for (unsigned i = 0; i < CORNERS; ++i) {
            const uint8_t *p = contours[style][i];
            const uint8_t *q = scene + (style * CORNERS + i) * 2;
            art->contour[style][i] = (BfmDialoguePoint){signed16(p), signed16(p + 2)};
            art->inset[style][i] = (BfmDialoguePoint){signed8(q[0]), signed8(q[1])};
        }
    const uint16_t *bubble = &tiles[447][352];
    art->paper = rgb(bubble[0]);
    art->highlight = rgb(bubble[1]);
    art->outline = rgb(bubble[2]);
    for (unsigned i = 0; i < 16; ++i)
        art->ink[i] = rgb(tiles[446][352+i]);
    for (unsigned y = 0; y < 16; ++y)
        for (unsigned x = 0; x < 16; ++x) {
            unsigned index = (tail[y * 64 + x/2] >> (4 * (x & 1))) & 15;
            art->tail[y][x] = bubble[index] ? rgb(bubble[index]) : 0;
        }
    for (unsigned y = 0; y < 12; ++y)
        for (unsigned x = 0; x < 12; ++x) {
            unsigned packed = tiles[256 + y][508 + x / 4];
            uint16_t color = tiles[446][352 + ((packed >> (4 * (x & 3))) & 15)];
            art->prompt[y][x] = color ? rgb(color) : 0;
        }
    /* Musashi is in the shared atlas. The other faces use consecutive
     * room-uploaded tiles and palettes, including Twinpeak's Rootrick. */
    for (unsigned face = 0; face < 4; ++face) {
        unsigned left = face ? 448 + 8 * face : 624;
        unsigned top = face ? 400 : 416;
        unsigned palette = face ? 456 + face : 296;
        for (unsigned y = 0; y < 40; ++y)
            for (unsigned x = 0; x < 32; ++x) {
                unsigned packed = tiles[top + y][left + x/4];
                uint16_t color = tiles[palette][352+((packed >> (4*(x & 3))) & 15)];
                art->portrait[face][y][x] = color ? rgb(color) : 0;
            }
    }
    return 1;
}

static BfmForestMessageArt *load_art(const MusashiDiscMedia *disc, const uint8_t *scene,
                                     size_t bytes, const uint16_t tiles[512][1024], size_t inset)
{
    if (!disc || !scene || !tiles || inset > bytes || bytes - inset < 4 * CORNERS)
        return NULL;
    BfmForestMessageArt *art = malloc(sizeof(*art));
    if (!art) return NULL;
    if (!read_art(art, disc, scene + inset, tiles)) {
        free(art);
        return NULL;
    }
    size_t after_contour = inset + 4 * CORNERS;
    art->has_tail_limits = bytes - after_contour >= sizeof art->tail_limits;
    if (art->has_tail_limits)
        for (unsigned i = 0; i < 6; ++i)
            art->tail_limits[i] = (int16_t)signed16(scene + after_contour + 2 * i);
    return art;
}

BfmForestMessageArt *bfm_forest_message_art_load(const MusashiDiscMedia *disc,
                                                const uint8_t *scene, size_t bytes,
                                                const BfmForestTerrain *terrain)
{
    return load_art(disc, scene, bytes, bfm_forest_terrain_tiles(terrain), FOREST_INSET);
}

BfmForestMessageArt *bfm_bedroom_message_art_load(const MusashiDiscMedia *disc,
                                                  const uint8_t *scene, size_t bytes,
                                                  const uint16_t tiles[512][1024])
{
    return load_art(disc, scene, bytes, tiles, BEDROOM_INSET);
}

BfmForestMessageArt *bfm_castle_message_art_load(const MusashiDiscMedia *disc, const uint8_t *scene,
                                                  size_t bytes, const uint16_t tiles[512][1024], size_t inset)
{
    return load_art(disc, scene, bytes, tiles, inset);
}

void bfm_forest_message_art_free(BfmForestMessageArt *art)
{
    free(art);
}

/* Both interpolation and scaling round toward the lesser pixel coordinate. */
static int floor_div(int value, int divisor)
{
    return value / divisor - (value % divisor < 0);
}

static int coordinate(int round, int pointed, int mix, int scale)
{
    return floor_div((round + floor_div((pointed - round) * mix, 4)) * scale, 4096);
}

static int bubble_scale_x(unsigned columns, int step)
{
    if (columns == 20 || columns == 28)
        return step * 512 * ((int)columns * 6 + 56) / 154;
    return step * 904;
}

static void draw_bubble(const BfmForestMessageArt *art, int center, int tail_x, int right,
                        const BfmForestMessageFrame *frame, uint32_t pixels[240][320])
{
    int step = (int)frame->border_step, mix = (int)frame->jagged_mix;
    int scale_x = bubble_scale_x(frame->text_columns, step);
    BfmDialoguePoint border[CORNERS], inset[CORNERS];
    for (unsigned i = 0; i < CORNERS; ++i) {
        BfmDialoguePoint a = art->contour[0][i], b = art->contour[1][i];
        if (right) {
            a.x = -a.x;
            b.x = -b.x;
        }
        border[i].x = center + coordinate(a.x, b.x, mix, scale_x);
        border[i].y = 44 + coordinate(a.y, b.y, mix, step * 4096 / 6);
        BfmDialoguePoint delta = art->inset[mix > 2][i];
        if (right) delta.x = -delta.x;
        inset[i] = (BfmDialoguePoint){border[i].x + delta.x, border[i].y + delta.y};
    }
    bfm_dialogue_contour(border, inset, art->paper, art->highlight, art->outline, 10, pixels);
    unsigned tail_width = right ? 16u : 15u;
    if (frame->text_columns == 20) {
        int width = floor_div((right ? -9 : 9) * scale_x, 4096);
        tail_width = (unsigned)abs(width);
    }
    if (!mix && step == 8 && tail_x >= 0 && tail_x <= 320 - (int)tail_width) {
        for (unsigned y = 0; y < 11; ++y)
            for (unsigned x = 0; x < tail_width; ++x) {
                int u = (int)((x * 15 + tail_width / 2) / tail_width);
                BfmDialoguePoint uv = {right ? 15 - u : u, (int)(y*15+5)/11};
                if (right && tail_width == 16)
                    uv = bfm_dialogue_reflected_texel(x, y, 16, 11, 15);
                uint32_t color = art->tail[uv.y][uv.x];
                if (color) pixels[76+y][tail_x+x] = color;
            }
    }
}

static void draw_portrait(const BfmForestMessageArt *art, int right, int left,
                          unsigned portrait, uint32_t pixels[240][320])
{
    if (!portrait || portrait > 4) return;
    unsigned face = portrait - 1;
    for (unsigned y = 0; y < 40; ++y)
        for (unsigned x = 0; x < 32; ++x) {
            BfmDialoguePoint uv = right ? bfm_dialogue_reflected_texel(x, y, 32, 40, 39) :
                                                      (BfmDialoguePoint){(int)x, (int)y};
            uint32_t color = art->portrait[face][uv.y][uv.x];
            if (color) pixels[24+y][left+x] = color;
        }
}

static void draw_notice(unsigned scale, uint32_t pixels[240][320])
{
    int width = (int)(232 * scale / 4096), height = (int)(56 * scale / 4096);
    int left = 160 - width/2, top = 44 - height/2;
    uint32_t outer = rgb(22 | (19 << 5) | (9 << 10));
    uint32_t inner = rgb(28 | (25 << 5) | (14 << 10));
    uint32_t paper = rgb(31 | (28 << 5) | (19 << 10));
    for (int y = 0; y <= height; ++y)
        for (int x = 0; x <= width; ++x) {
            int edge_x = x < width-x ? x : width-x, edge_y = y < height-y ? y : height-y;
            int edge = edge_x < edge_y ? edge_x : edge_y;
            pixels[top+y][left+x] = !edge ? outer : edge == 1 ? inner : paper;
        }
}

static void draw_choice(int left, unsigned columns, unsigned answer, uint32_t pixels[240][320])
{
    int top = 24 + 14*((int)answer - 1);
    for (int y = 0; y < FONT_HEIGHT; ++y) {
        int weight = y <= 5 ? y : FONT_HEIGHT-y;
        int span = y <= 5 ? 5 : FONT_HEIGHT-5;
        /* Two half-blended Gouraud strips meet at the white ridge. */
        unsigned red = (unsigned)(57 + (255-57)*weight/span) / 8;
        unsigned blue = (unsigned)(113 + (255-113)*weight/span) / 8;
        for (int x = left; x < left + (int)columns * 6; ++x) {
            uint32_t dst = pixels[top+y][x];
            unsigned r = (((dst >> 19) & 31) + red) / 2;
            unsigned g = (((dst >> 11) & 31) + 31) / 2;
            unsigned b = (((dst >> 3) & 31) + blue) / 2;
            pixels[top+y][x] = rgb((uint16_t)(r | g << 5 | b << 10));
        }
    }
}

static void draw_message(const BfmForestMessageArt *art, int tail_x, int right, int staged,
                         int draw_prompt, const BfmForestMessageFrame *frame, uint32_t pixels[240][320])
{
    if (!art || !frame || !pixels || !frame->border_step || frame->border_step > 8 ||
        frame->jagged_mix > 4 || frame->scroll > 9)
        return;
    unsigned columns = frame->text_columns == 20 || frame->text_columns == 28 ? frame->text_columns : 36;
    int side_offset = staged && columns == 20 ? (right ? 64 : -64) : 0;
    int center = 160 + side_offset;
    int reflect = right;
    if (side_offset) {
        /* The small staged balloon faces inward. Its pointer starts at
         * its own center irrespective of the speaker's subject offset. */
        reflect = !right;
        tail_x = center + (reflect ? floor_div(-9 * bubble_scale_x(columns, 8), 4096) : 0);
    }
    if (frame->style == 1) draw_notice(frame->border_scale, pixels);
    else draw_bubble(art, center, tail_x, reflect, frame, pixels);
    if (!frame->text_visible) return;
    int inset = (36 - (int)columns) * 3;
    int face_x = side_offset + (right ? 256 - inset : 32 + inset);
    if (frame->portrait) draw_portrait(art, right, face_x, frame->portrait, pixels);
    int text_x = frame->style == 1 ? 52 : (right ? 32 : 72) + inset + side_offset;
    if (frame->choices && frame->answer && frame->answer <= 3)
        draw_choice(text_x, columns, frame->choice_row + frame->answer, pixels);
    for (int row = 0; row < 3; ++row)
        for (unsigned col = 0; col < columns; ++col) {
            BfmForestMessageCell cell = frame->cells[row][col];
            if (cell.glyph < 32 || cell.glyph >= 32 + GLYPHS || !cell.ink || cell.ink >= 16)
                continue;
            const uint8_t *glyph = art->glyph[cell.glyph-32];
            for (int y = 0; y < FONT_HEIGHT; ++y) {
                int screen_y = 24 + 14 * row + y - (int)frame->scroll;
                if (screen_y < 24) continue;
                for (unsigned x = 0; x < 6; ++x)
                    if (glyph[y] & (128u >> x))
                        pixels[screen_y][text_x + 6 * col + x] = art->ink[cell.ink];
            }
        }
    if (draw_prompt && frame->inline_prompt && frame->prompt_row < 3 && frame->prompt_column < columns) {
        for (unsigned y = 0; y < 12; ++y)
            for (unsigned x = 0; x < 12; ++x)
                if (art->prompt[y][x])
                    pixels[24 + 14 * frame->prompt_row + y][text_x + 6 * frame->prompt_column + x] =
                        art->prompt[y][x];
    }
}

void bfm_forest_message_art_draw(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                 uint32_t pixels[240][320])
{
    draw_message(art, 160, 0, 0, frame && frame->style == 1, frame, pixels);
}

void bfm_bedroom_message_art_draw(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                  int speaker_x, uint32_t pixels[240][320])
{
    bfm_bedroom_message_art_draw_side(art, frame, speaker_x, 1, pixels);
}

void bfm_bedroom_message_art_draw_side(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                       int speaker_x, int portrait_on_right, uint32_t pixels[240][320])
{
    int left = portrait_on_right ? speaker_x - 17 : speaker_x;
    draw_message(art, left, portrait_on_right, 1, 1, frame, pixels);
}

int bfm_forest_message_art_draw_placed(const BfmForestMessageArt *art, const BfmForestMessageFrame *frame,
                                       int16_t offset, int on_right, uint32_t pixels[240][320])
{
    if (!art || !frame || !pixels || !art->has_tail_limits || (on_right != 0 && on_right != 1))
        return 0;
    int scale = bubble_scale_x(frame->text_columns, 8);
    if (on_right) scale = -scale;
    int x = offset * 4096 / scale;
    if (x) {
        /* The close-up script locates its tail in balloon-local space. The
         * room's two rim bands limit how far the pointer can move. */
        const int16_t *limits = art->tail_limits;
        int inner = x > limits[1] && x < limits[4];
        int low = limits[inner ? 2 : 0], high = limits[inner ? 3 : 5];
        if (x < low) x = low;
        if (x > high) x = high;
    }
    int anchor = 160 + on_right + floor_div(x * scale, 4096);
    bfm_bedroom_message_art_draw_side(art, frame, anchor, on_right, pixels);
    return 1;
}

int bfm_library_question_draw(const BfmForestMessageArt *art, const uint8_t *box, const uint8_t *portrait,
                              const uint16_t tiles[512][1024], const uint8_t *text, size_t bytes,
                              unsigned update, uint32_t pixels[240][320])
{
    if (!art || !box || !portrait || !tiles || !text || !pixels) return 0;
    int x=160+signed16(box), y=120+signed16(box+2);
    int width=16*signed16(box+4)+4, height=14*signed16(box+6)+14;
    if (bytes<3 || text[0]!=2 || text[1] || text[2] ||
        width<=0 || width>320 || height<=0 || height>240) return 0;
    BfmDialoguePoint border[CORNERS], inset[CORNERS];
    for (unsigned i=0;i<CORNERS;++i) {
        BfmDialoguePoint point=art->contour[0][i];
        border[i].x=x+floor_div(point.x*(width*4096/154),4096);
        border[i].y=y+floor_div(point.y*(height*4096/42),4096);
        inset[i].x=border[i].x+art->inset[0][i].x;
        inset[i].y=border[i].y+art->inset[0][i].y;
    }
    bfm_dialogue_contour(border,inset,art->paper,art->highlight,art->outline,10,pixels);
    int px=160+signed16(portrait), py=120+signed16(portrait+2);
    unsigned w=le16(portrait+4),h=le16(portrait+6),u=le16(portrait+8),v=le16(portrait+10);
    unsigned clut=le16(portrait+12), page=le16(portrait+14);
    if (w>64 || h>64 || u+w>256 || v+h>256 || page>31 || px<0 || py<0 ||
        px+(int)w>320 || py+(int)h>240 || (clut>>6)>=512) return 0;
    for (unsigned j=0;j<h;++j) for (unsigned i=0;i<w;++i) {
        unsigned packed=tiles[((page>>4)&1)*256+v+j][(page&15)*64+(u+i)/4];
        uint16_t color=tiles[clut>>6][(clut&63)*16+((packed>>(4*((u+i)&3)))&15)];
        if (color) pixels[py+j][px+i]=rgb(color);
    }
    unsigned row=0,col=0,ink=1;
    int ox=160+signed16(box+8)-64, oy=120+signed16(box+10)-20;
    for (size_t i=3;i<bytes;++i) {
        unsigned c=text[i];
        if (!c) return 1;
        if (c=='\n') {++row;col=0;continue;}
        if (c==1 && i+1<bytes) {ink=text[++i];continue;}
        if (c<32 || c>126 || row>=3 || col>=36 || ink>=16) return 0;
        int gx=ox+6*(int)col++,gy=oy+14*(int)row;
        if (gx<0 || gx+6>320 || gy<0 || gy+12>240) return 0;
        if (update<4+row) continue;
        for (int j=0;j<12;++j) for (int k=0;k<6;++k)
            if (art->glyph[c-32][j]&(128u>>k)) pixels[gy+j][gx+k]=art->ink[ink];
    }
    return 0;
}
