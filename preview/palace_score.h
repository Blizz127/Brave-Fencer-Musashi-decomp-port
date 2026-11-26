/* SPDX-License-Identifier: MIT */
#ifndef BFM_PALACE_SCORE_H
#define BFM_PALACE_SCORE_H
#include "musashi_disc_media.h"

/* Render the dry stereo entrance from score tick zero. The caller frees the
 * newly allocated interleaved stereo output. Both the device rate (44100 Hz)
 * and preview mixer rate (37800 Hz) are supported. An unsupported controller
 * or missing mounted instrument leaves the output empty. */
int bfm_palace_score_render(const MusashiDiscMedia *disc, unsigned rate, size_t frames,
                            int16_t **stereo);
#endif
