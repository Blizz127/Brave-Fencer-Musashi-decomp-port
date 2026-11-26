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
enum { BFM_OPENING_LAST_FIELD = 8316 };
enum { BFM_OPENING_END, BFM_OPENING_FRAME, BFM_OPENING_FOREST_PENDING };
/* A fresh Cross edge selects the authored chapter transition. Returns one
 * when accepted, zero for an already selected route and negative on failure. */
int bfm_opening_request_skip(void *scene, const MusashiDiscMedia *disc, unsigned fields,
                             char *error, size_t error_size);
int bfm_opening_skip_member(const void *scene, unsigned *member);
/* FRAME produces an image. On END, render the final opening field.
 * FOREST_PENDING has already painted the black end of the chapter card. */
int bfm_opening_frame(void *scene, unsigned int fields, uint32_t *pixels,
                      int width, int height);
/* Render the final supported field, even when the display clock skipped it. */
int bfm_opening_final_frame(void *scene, uint32_t *pixels, int width, int height);
int bfm_opening_voice_due(void *scene, unsigned field, unsigned *bank, unsigned *cue);
int bfm_opening_voice_schedule(void *scene, unsigned field, unsigned *bank, unsigned *cue,
                              unsigned *issue_field);
int bfm_opening_voice_started(void *scene, unsigned bank, unsigned cue, unsigned field);
void bfm_opening_free(void *scene);

#endif
