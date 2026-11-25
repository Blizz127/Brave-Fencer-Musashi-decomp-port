/* SPDX-License-Identifier: MIT */
#include "scene_views.h"
#include "scene_tiles.h"
#include <stdio.h>
#include <stdlib.h>

enum { VRAM_WIDTH = 1024, VRAM_HEIGHT = 512 };
enum { SCREEN_WIDTH = 640, SCREEN_HEIGHT = 480, PICTURE_HEIGHT = 440 };
enum { PAGE_LEFT = 640, PALETTE_TOP = 480 };

typedef struct {
    uint16_t words[VRAM_HEIGHT][VRAM_WIDTH];
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

int bfm_title_frame(void *scene, unsigned int fields, unsigned int phase,
                    unsigned int choice, int prompt_visible, uint32_t *pixels,
                    int width, int height)
{
    const TitleImage *image = scene;
    (void)fields;
    if (!image || !pixels || phase > 1 || choice > 1 ||
        !((width == SCREEN_WIDTH && height == SCREEN_HEIGHT) ||
          (width == SCREEN_WIDTH / 2 && height == SCREEN_HEIGHT / 2)))
        return 0;
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
