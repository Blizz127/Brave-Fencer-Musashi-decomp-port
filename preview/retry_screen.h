/* SPDX-License-Identifier: MIT */
#ifndef BFM_RETRY_SCREEN_H
#define BFM_RETRY_SCREEN_H

#include "forest_field.h"
#include "forest_hud.h"
#include "forest_message_art.h"
#include "forest_voice.h"

typedef struct BfmRetryScreen BfmRetryScreen;
typedef enum {
    BFM_RETRY_AUDIO_NONE, BFM_RETRY_AUDIO_FADE_FIELD, BFM_RETRY_AUDIO_START, BFM_RETRY_AUDIO_FADE_OUT, BFM_RETRY_AUDIO_RESTORE_ROOM,
    BFM_RETRY_AUDIO_STOP_OPTICAL
} BfmRetryAudioEvent;

/* The active field, disc and shared playthrough outlive this presentation. */
BfmRetryScreen *bfm_retry_screen_open(const MusashiDiscMedia *disc, const BfmForestField *field,
                                      uint16_t room_id, BfmGameplayRun *gameplay,
                                      const BfmForestCamera *last_camera, const int32_t last_target[3],
                                      const int16_t feet[3],
                                      unsigned yaw, uint32_t visible_parts);
/* Refresh the original field camera and its subject before the room is hidden.
 * The field heading tracker can sample the turning player's current yaw. */
void bfm_retry_screen_update_outgoing_view(BfmRetryScreen *screen, const BfmForestCamera *view,
                                          const int32_t target[3]);
unsigned bfm_retry_screen_yaw(const BfmRetryScreen *screen);
/* One normal 30 Hz game update. Returns 0 while active, 1 when the caller
 * may transfer to the checkpoint, or -1 on a mounted-service failure. */
int bfm_retry_screen_step(BfmRetryScreen *screen, unsigned held_pad, unsigned pressed_pad);
/* Render the outgoing world before draw while this predicate is true. */
int bfm_retry_screen_keep_world(const BfmRetryScreen *screen);
int bfm_retry_screen_draw(const BfmRetryScreen *screen, const BfmForestHud *hud,
                          const BfmForestMessageArt *art, uint32_t pixels[240][320]);
/* Consume the score command issued by the last game step, if any. */
BfmRetryAudioEvent bfm_retry_screen_audio_event(BfmRetryScreen *screen);
ForestVoice *bfm_retry_screen_voice(const BfmRetryScreen *screen);
unsigned bfm_retry_screen_generation(const BfmRetryScreen *screen);
void bfm_retry_screen_free(BfmRetryScreen *screen);
#endif
