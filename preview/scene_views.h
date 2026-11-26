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
enum { BFM_OPENING_LAST_FIELD = 17533 };
enum { BFM_OPENING_END, BFM_OPENING_FRAME, BFM_OPENING_FOREST_PENDING };
/* A fresh Cross edge selects the authored chapter transition. Returns one
 * when accepted, zero for an already selected route and negative on failure. */
int bfm_opening_request_skip(void *scene, const MusashiDiscMedia *disc, unsigned fields,
                             char *error, size_t error_size);
/* Step the completed court dialogue's automatic departure independently of
 * input. Returns one on the initiating update, zero otherwise, negative on failure. */
int bfm_opening_advance_court(void *scene, const MusashiDiscMedia *disc, unsigned field,
                             char *error, size_t error_size);
struct BfmGameplayRun;
void bfm_opening_advance_playtime(void *scene, unsigned field, struct BfmGameplayRun *run);
struct BfmForestWalk;
/* Take the selected forest after the card ends. Natural entry is prepared
 * during the fade and supplies its opening-clock field. Cross uses a new
 * forest clock (zero). Ownership transfers to the caller exactly once. */
struct BfmForestWalk *bfm_opening_take_forest(void *scene, const MusashiDiscMedia *disc,
                                             unsigned field, uint64_t *forest_field);
/* The chapter clock supplies the incoming music origin during the final black interval. */
int bfm_opening_forest_score_field(const void *scene, unsigned field, uint64_t *start_field);
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
