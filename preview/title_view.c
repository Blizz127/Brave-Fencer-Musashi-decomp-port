/* SPDX-License-Identifier: MIT */
#include "scene_views.h"
#include "scene_tiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { VRAM_WIDTH = 1024, VRAM_HEIGHT = 512 };
enum { SCREEN_WIDTH = 640, SCREEN_HEIGHT = 480, PICTURE_HEIGHT = 440 };
enum { PAGE_LEFT = 640, PALETTE_TOP = 480 };

typedef struct {
    uint16_t words[VRAM_HEIGHT][VRAM_WIDTH];
    const MusashiDiscMedia *disc;
    uint16_t loading_colors[16];
    unsigned char loading_pixels[1024];
    int loading_ready;
} TitleImage;

int bfm_title_load(const MusashiDiscMedia *disc, void **scene, char *error,
                   size_t error_size)
{
    if (!scene)
        return 0;
    *scene = NULL;
    TitleImage *image = calloc(1, sizeof(*image));
    if (!image || !disc || !bfm_scene_read_tiles(disc, "MAIN.CD", 3, image->words)) {
        free(image);
        if (error && error_size)
            snprintf(error, error_size, "Could not read the title graphics from MAIN.CD on the US disc.");
        return 0;
    }
    image->disc = disc;
    *scene = image;
    return 1;
}

static uint16_t texel(const TitleImage *image, unsigned x, unsigned y,
                      unsigned palette)
{
    uint16_t pair = image->words[y][PAGE_LEFT + x / 2];
    unsigned index = (pair >> ((x & 1) * 8)) & 255;
    return image->words[PALETTE_TOP + palette][index];
}

/* Sprite texels with all bits clear leave the picture underneath visible.
 * The high bit belongs to the texture processor, not the visible RGB value. */
static uint16_t title_pixel(const TitleImage *image, unsigned x, unsigned y,
                            unsigned phase, unsigned choice, int prompt_visible)
{
    uint16_t background = texel(image, x, y + 36, 3);
    uint16_t overlay = 0;
    if (phase == 0 && prompt_visible && x >= 144 && x < 496 && y >= 304 && y < 336) {
        overlay = texel(image, x - 144, y - 304, 2);
    } else if (phase == 1) {
        unsigned selected_y = choice ? 320 : 288;
        if (x >= 256 && x < 384 && y >= 288 && y < 352)
            overlay = texel(image, x - 256 + 640, y - 288, 10);
        else if (y >= selected_y && y < selected_y + 32) {
            if (x >= 208 && x < 240)
                overlay = texel(image, x - 208 + 384, y - selected_y, 0);
            else if (x >= 400 && x < 432)
                overlay = texel(image, x - 400 + 576, y - selected_y, 0);
        }
    }
    return overlay ? overlay : background;
}

static uint32_t rgb555(uint16_t pixel)
{
    uint32_t r = (pixel & 31u) * 255 / 31;
    uint32_t g = ((pixel >> 5) & 31u) * 255 / 31;
    uint32_t b = ((pixel >> 10) & 31u) * 255 / 31;
    return UINT32_C(0xff000000) | r << 16 | g << 8 | b;
}

static uint32_t little_word(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* Member 4 of the supported US disc holds the loading CLUT and sprite
 * immediately before their upload descriptors. Verify the dimensions and
 * their adjacency before accepting either resource. */
static int load_loading_sprite(TitleImage *image)
{
    enum { RESOURCE_OFFSET = 0x68a4, PALETTE_BYTES = 32, PIXEL_BYTES = 1024,
           RESOURCE_BYTES = PALETTE_BYTES + PIXEL_BYTES, SECTOR_BYTES = 2048 };
    unsigned char record[8], section[RESOURCE_BYTES + 48];
    size_t file_size;
    if (!musashi_disc_media_read_root_range(image->disc, "MAIN.CD", 8 + 4 * 8,
                                            record, sizeof(record), &file_size))
        return 0;
    uint32_t sector = little_word(record);
    uint32_t extent = little_word(record + 4);
    if (!sector || sector > file_size / SECTOR_BYTES)
        return 0;
    size_t start = (size_t)sector * SECTOR_BYTES;
    if (extent > file_size - start || extent < RESOURCE_OFFSET + sizeof(section) ||
        !musashi_disc_media_read_root_range(image->disc, "MAIN.CD", start + RESOURCE_OFFSET,
                                            section, sizeof(section), &file_size))
        return 0;
    const unsigned char *palette_record = section + RESOURCE_BYTES;
    const unsigned char *pixel_record = palette_record + 16;
    uint32_t palette_address = little_word(palette_record + 12);
    uint32_t pixel_address = little_word(pixel_record + 12);
    if (little_word(palette_record) != 9 || little_word(palette_record + 8) != (1u << 16 | 16u) ||
        little_word(pixel_record) != 0 || little_word(pixel_record + 8) != (8u << 16 | 64u) ||
        pixel_address < palette_address || pixel_address - palette_address != PALETTE_BYTES ||
        little_word(pixel_record + 16) != 255)
        return 0;
    for (unsigned i = 0; i < 16; ++i)
        image->loading_colors[i] = (uint16_t)(section[2 * i] | (uint16_t)section[2 * i + 1] << 8);
    memcpy(image->loading_pixels, section + PALETTE_BYTES, PIXEL_BYTES);
    image->loading_ready = 1;
    return 1;
}

static int loading_frame(TitleImage *image, uint32_t *pixels, int width, int height)
{
    if (!image->loading_ready && !load_loading_sprite(image))
        return 0;
    const unsigned scale = (unsigned)width / 320;
    for (size_t i = 0; i < (size_t)width * (size_t)height; ++i)
        pixels[i] = UINT32_C(0xff000000);
    /* The 128 x 16 sprite is stored as two 128 x 8 halves side by side. */
    for (unsigned y = 0; y < 16; ++y) {
        for (unsigned x = 0; x < 128; ++x) {
            unsigned offset = (y % 8) * 128 + (y / 8) * 64 + x / 2;
            unsigned index = (image->loading_pixels[offset] >> (4 * (x & 1))) & 15;
            uint32_t color = rgb555(image->loading_colors[index]);
            for (unsigned dy = 0; dy < scale; ++dy) {
                uint32_t *out = pixels + ((200 + y) * scale + dy) * (unsigned)width;
                for (unsigned dx = 0; dx < scale; ++dx)
                    out[(176 + x) * scale + dx] = color;
            }
        }
    }
    return 1;
}

int bfm_title_frame(void *scene, unsigned int fields, unsigned int phase,
                    unsigned int choice, int prompt_visible, uint32_t *pixels,
                    int width, int height)
{
    TitleImage *image = scene;
    (void)fields;
    if (!image || !pixels || phase > 2 || choice > 1 ||
        !((width == SCREEN_WIDTH && height == SCREEN_HEIGHT) ||
          (width == SCREEN_WIDTH / 2 && height == SCREEN_HEIGHT / 2)))
        return 0;
    if (phase == 2)
        return loading_frame(image, pixels, width, height);
    unsigned scale = SCREEN_WIDTH / (unsigned)width;
    for (int y = 0; y < height; ++y) {
        unsigned screen_y = (unsigned)y * scale + scale - 1;
        for (int x = 0; x < width; ++x) {
            unsigned screen_x = (unsigned)x * scale + scale - 1;
            uint16_t color = 0;
            if (screen_y >= 20 && screen_y < 20 + PICTURE_HEIGHT)
                color = title_pixel(image, screen_x, screen_y - 20, phase, choice, prompt_visible);
            pixels[(size_t)y * (size_t)width + (size_t)x] = rgb555(color);
        }
    }
    return 1;
}

void bfm_title_free(void *scene)
{
    free(scene);
}
