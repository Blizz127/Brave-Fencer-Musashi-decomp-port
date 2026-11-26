/* SPDX-License-Identifier: MIT */
#ifndef BFM_TOWER_SUMMIT_ARRIVAL_H
#define BFM_TOWER_SUMMIT_ARRIVAL_H

#include "forest_message_art.h"
#include "forest_speech_actor.h"
#include "forest_voice.h"

typedef struct {
    BfmForestSpeechActor actor;
    BfmForestMessage *page;
    const uint8_t *speech, *rim;
    size_t speech_bytes;
    uint32_t parts;
    int16_t scale[3]; /* Actor's local axes in Q12. */
    unsigned phase, age;
} BfmTowerSummitArrival;

enum { BFM_SUMMIT_ARRIVAL_ERROR, BFM_SUMMIT_ARRIVAL_LOCKED, BFM_SUMMIT_ARRIVAL_RELEASED };

/* Cold arrival through the lower tower's bell exit. The field and mounted
 * resources must outlive this presenter. Begin with an empty presenter. */
int bfm_tower_summit_arrival_begin(BfmTowerSummitArrival *arrival, const BfmForestField *room,
                                   const MusashiDiscMedia *disc, uint32_t parts, BfmForestPose *pose);
/* One game update. Advance the shared voice transport before calling. Pass the
 * room's transition completion and fresh focused button edges. */
int bfm_tower_summit_arrival_tick(BfmTowerSummitArrival *arrival, ForestVoice *voice, unsigned *generation,
                                  int entrance_ready, int confirm, int reveal, BfmForestPose *pose);
void bfm_tower_summit_arrival_close(BfmTowerSummitArrival *arrival);
BfmForestMessageArt *bfm_tower_summit_arrival_art(const BfmForestField *room, const MusashiDiscMedia *disc);
void bfm_tower_summit_arrival_draw_light(const BfmTowerSummitArrival *arrival, const BfmForestCamera *camera,
                                        const int16_t xyz[3], unsigned parity, uint32_t pixels[240][320]);
void bfm_tower_summit_arrival_draw(const BfmTowerSummitArrival *arrival, const BfmForestMessageArt *art,
                                  int speaker_x, uint32_t pixels[240][320]);

#endif
