/* SPDX-License-Identifier: MIT */
#ifndef BFM_SCENE_TILES_H
#define BFM_SCENE_TILES_H
#include "musashi_disc_media.h"
#include <stdint.h>

/* Read the first sparse VRAM upload in a member of a mounted game archive.
 * The destination is zero-filled by its caller. It may be partially populated
 * on failure and must then be discarded. */
int bfm_scene_read_tiles(const MusashiDiscMedia *disc, const char *archive,
                         unsigned member, uint16_t words[512][1024]);
#endif
