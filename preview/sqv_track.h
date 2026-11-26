/* SPDX-License-Identifier: MIT */
#ifndef BFM_SQV_TRACK_H
#define BFM_SQV_TRACK_H

#include "musashi_disc_media.h"

typedef enum {
    BFM_SQV_OK, BFM_SQV_INVALID, BFM_SQV_UNSUPPORTED, BFM_SQV_DISC_ERROR, BFM_SQV_NO_MEMORY
} BfmSqvResult;

typedef enum {
    BFM_SQV_TEMPO, BFM_SQV_PROGRAM, BFM_SQV_NOTE_ON, BFM_SQV_NOTE_OFF,
    BFM_SQV_PAN, BFM_SQV_CONTROL, BFM_SQV_END, BFM_SQV_PITCH_BEND
} BfmSqvEventKind;

typedef struct {
    uint32_t tick, tempo_us_per_quarter;
    BfmSqvEventKind kind;
    /* number is the key, program, controller or compact pitch wheel (64
     * centered). value is the note-on velocity or controller value, zero
     * for pitch wheel. Tempo and end have channel 0xff. */
    uint8_t channel, number, value;
} BfmSqvEvent;

typedef struct {
    BfmSqvEvent *events;
    size_t event_count;
    uint32_t duration_ticks;
    uint16_t ticks_per_quarter;
} BfmSqvTrack;

/* Read one zero-based score from SC01.CD member 2. Equal-tick events retain
 * their order. CONTROL retains uninterpreted controller number and value.
 * Use a fresh or freed output. Failure leaves it empty. */
BfmSqvResult bfm_sqv_track_load(const MusashiDiscMedia *disc, unsigned track, BfmSqvTrack *out);
/* Same ownership and ordering for supported SC01.CD members: the three
 * palace tracks in member 2 and the single forest track in member 78. */
BfmSqvResult bfm_sqv_member_track_load(const MusashiDiscMedia *disc, unsigned member,
                                      unsigned track, BfmSqvTrack *out);
void bfm_sqv_track_free(BfmSqvTrack *track);

#endif
