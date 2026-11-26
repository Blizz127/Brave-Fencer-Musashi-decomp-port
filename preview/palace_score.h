/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_SCORE_H
#define BFM_PALACE_SCORE_H
#include "musashi_disc_media.h"

/* Opening fields share the scene's first palace image as field zero. */
enum {
    BFM_PALACE_SCORE_FIELD = 4630,
    BFM_PALACE_DEPARTURE_FIELD = 17466,
    BFM_PALACE_SCORE_END_FIELD = 17560
};

/* Render the mounted dry score through the room fade. The caller frees
 * the allocated interleaved stereo output. Both the device rate (44100 Hz)
 * and preview mixer rate (37800 Hz) are supported. An unsupported controller
 * or missing mounted instrument leaves the output empty. */
int bfm_palace_score_render(const MusashiDiscMedia *disc, unsigned rate, size_t frames,
                            int16_t **stereo);
#endif
