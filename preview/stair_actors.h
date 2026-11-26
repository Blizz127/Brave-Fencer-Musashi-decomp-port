/* SPDX-License-Identifier: MIT */
#ifndef BFM_STAIR_ACTORS_H
#define BFM_STAIR_ACTORS_H
#include "musashi_disc_media.h"
#include <stddef.h>
#include <stdint.h>

typedef struct BfmStairActors BfmStairActors;
/* The validated scene storage is borrowed until the actors are freed. */
BfmStairActors *bfm_stair_actors_load(const MusashiDiscMedia *disc, const uint8_t *scene, size_t bytes);
void bfm_stair_actors_free(BfmStairActors *actors);
void bfm_stair_actors_prepare(BfmStairActors *actors, unsigned field,
                              const int axes[3][3], const int origin[3], const int eye[3]);
void bfm_stair_actors_draw_to(BfmStairActors *actors, int depth,
                             const uint16_t tiles[512][1024], uint32_t *pixels);
#endif
