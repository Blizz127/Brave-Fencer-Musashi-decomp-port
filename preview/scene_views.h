/* SPDX-License-Identifier: MIT */
#ifndef BFM_SCENE_VIEWS_H
#define BFM_SCENE_VIEWS_H

#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

/* The scene belongs to its caller until freed; the disc must outlive it.
 * The title is 640 x 480 and the opening is 320 x 240. Both present
 * full frames as 0xAARRGGBB host integers. */
int bfm_title_load(const MusashiDiscMedia *disc, void **scene, char *error,
                   size_t error_size);
int bfm_title_frame(void *scene, unsigned int fields, unsigned int phase,
                    unsigned int choice, int prompt_visible, uint32_t *pixels,
                    int width, int height);
void bfm_title_free(void *scene);

int bfm_opening_load(const MusashiDiscMedia *disc, void **scene, char *error,
                     size_t error_size);
/* Positive produces a frame, zero ends the opening segment, and negative
 * reports a render failure. Retain the last presented image at the end. */
int bfm_opening_frame(void *scene, unsigned int fields, uint32_t *pixels,
                      int width, int height);
void bfm_opening_free(void *scene);

#endif
