/* SPDX-License-Identifier: MIT */
#ifndef BFM_RETRY_SCREEN_H
#define BFM_RETRY_SCREEN_H

#include "forest_field.h"
#include "forest_hud.h"
#include "forest_message_art.h"
#include "forest_voice.h"
#include "woods_retry.h"

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
enum { BFM_RETRY_ROOM_DATA_BYTES = 0x9c + 3 * 16 };
/* The Cleft Woods field draws its mounted hero from the MAIN motion frame
 * including its header. The field and its live texture pages outlive the screen. */
typedef struct {
    void *context;
    /* Brightness added by the outgoing scene wipe on each field update. */
    uint8_t wipe_step;
    int (*draw_actor)(void *, const BfmForestCamera *, uint32_t frame_address,
                      unsigned yaw, unsigned pitch, uint32_t visible,
                      const BfmForestModelLighting *lighting,
                      uint32_t pixels[240][320]);
} BfmWoodsRetryPresentation;
typedef struct {
    BfmForestTerrain *terrain;
    const MusashiForestPlayerMesh *hero;
    const uint8_t *data;
    size_t bytes;
    /* Optional recovery on the update after the defeat dialogue closes. */
    void (*restore_status)(BfmForestStatus *);
    /* Cleft Woods supplies its renderer and asks for the field recovery choices. */
    const BfmWoodsRetryPresentation *woods;
} BfmRetryMountedRoom;

/* The active room supplies its bounded defeat dialogue/light records at
 * open. Its textured terrain and hero mesh must outlive the screen.
 * The room owns its checkpoint, score policy and live resource recovery. */
BfmRetryScreen *bfm_retry_screen_open_mounted(const MusashiDiscMedia *disc,
                                             const BfmRetryMountedRoom *room,
                                             BfmGameplayRun *gameplay,
                                             const BfmForestCamera *last_camera,
                                             const int32_t last_target[3], const int16_t feet[3],
                                             unsigned yaw, uint32_t visible_parts);
/* Refresh the original field camera and its subject before the room is hidden.
 * The field heading tracker can sample the turning player's current yaw. */
void bfm_retry_screen_update_outgoing_view(BfmRetryScreen *screen, const BfmForestCamera *view,
                                          const int32_t target[3]);
unsigned bfm_retry_screen_yaw(const BfmRetryScreen *screen);
/* One normal 30 Hz game update. Returns 0 while active, 1 when the caller
 * may transfer to the checkpoint, or -1 on a mounted-service failure. */
int bfm_retry_screen_step(BfmRetryScreen *screen, unsigned held_pad, unsigned pressed_pad);
/* The field's chooser uses fresh direction, last-row shortcut and accept
 * edges. The field retains its screen until the receiving room takes ownership. */
int bfm_retry_screen_choose_step(BfmRetryScreen *screen, unsigned held_pad, unsigned pressed_pad,
                                 int direction, int cancel, int accept);
/* Report the actual result of the issued Woods score command before the
 * next game update. A failed required mount or release ends the scene. */
int bfm_retry_screen_woods_score_result(BfmRetryScreen *screen, BfmRetryAudioEvent event,
                                       int accepted);
/* INVALID until the scene and its optical service have released the field. */
BfmWoodsRetryPage bfm_retry_screen_woods_destination(const BfmRetryScreen *screen);
/* Events from the last update for the field's desktop audio owner. */
int bfm_retry_screen_voice_stopped(const BfmRetryScreen *screen);
int bfm_retry_screen_choice_changed(const BfmRetryScreen *screen);
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
