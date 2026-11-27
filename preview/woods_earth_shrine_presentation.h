/* SPDX-License-Identifier: MIT */
#ifndef BFM_WOODS_EARTH_SHRINE_PRESENTATION_H
#define BFM_WOODS_EARTH_SHRINE_PRESENTATION_H

#include "musashi_disc_media.h"

typedef struct BfmEarthPresentationMedia BfmEarthPresentationMedia;
typedef struct {
    BfmEarthPresentationMedia *media;
    unsigned phase, member, settle, last_player_member;
    int direct_return, returned, dark;
} BfmEarthShrinePresentation;

/* Start zeroed per visit. Release owned disc data when leaving the room. */
void bfm_earth_presentation_clear(BfmEarthShrinePresentation *p);
/* BEFORE and AFTER install player services for SC03/60 and /61. Admit the
 * command now; the actual resource request starts on a subsequent update. */
int bfm_earth_presentation_request(BfmEarthShrinePresentation *p, unsigned member);
/* Before the room's actor/script pass, once per effective player update. A
 * zero result reports a failed read or invalid archive. The previous tiles,
 * body and bank stay resident and the next update can retry the request. */
int bfm_earth_presentation_step(BfmEarthShrinePresentation *p, const MusashiDiscMedia *disc,
    uint16_t vram[512][1024], unsigned *resident_member);
int bfm_earth_presentation_player_ready(const BfmEarthShrinePresentation *p);
/* The later room predicate requests /61 again and waits for that specific
 * transfer. It does not install a player action. */
int bfm_earth_presentation_wait_return(BfmEarthShrinePresentation *p);
/* Set from the player award's actual enter/restore pulses, independently
 * of both resource workers. The room suppresses terrain and sky, plus the
 * upper two model and sprite groups, while retaining Musashi, medal and UI. */
void bfm_earth_presentation_mode(BfmEarthShrinePresentation *p, int entering);
int bfm_earth_presentation_dark(const BfmEarthShrinePresentation *p);
/* Borrowed mounted contents. The award body lasts until room teardown. The
 * physical slot 3 bank view lasts until the next successful /61 or teardown. */
const uint8_t *bfm_earth_presentation_body(const BfmEarthShrinePresentation *p, size_t *bytes);
int bfm_earth_presentation_bank(const BfmEarthShrinePresentation *p,
    const uint8_t **header, size_t *header_bytes, const uint8_t **samples, size_t *sample_bytes);

#endif
