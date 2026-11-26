/* SPDX-License-Identifier: MIT */
#include "forest_voice.h"
#include "disc_voice.h"
#include <stdlib.h>
#include <string.h>

enum {
    /* Two room updates enqueue and dispatch the optical request. The
     * warm-mode seek then takes thirteen service intervals to reach ReadS. */
    FIRST_READ_UPDATE = 2 + 13,
    /* The drive reads all eight interleaved channels at double speed. */
    SECTORS_PER_SECOND = 150
};

struct ForestVoice {
    DiscVoice *source;
    BfmDiscVoiceArchive archive;
    unsigned group_head_frames;
    unsigned status, until_read, remaining_ticks;
    size_t frames_left, lead_in;
};

static ForestVoice *load_voice(const MusashiDiscMedia *disc, BfmDiscVoiceArchive archive)
{
    ForestVoice *v = calloc(1, sizeof(*v));
    if (!v) return NULL;
    v->archive = archive;
    v->source = bfm_disc_voice_load(disc, archive);
    if (!v->source) {
        free(v);
        return NULL;
    }
    return v;
}

ForestVoice *bfm_forest_voice_load(const MusashiDiscMedia *disc)
{
    return load_voice(disc, BFM_VOICE_ST01);
}

ForestVoice *bfm_bedroom_voice_load(const MusashiDiscMedia *disc)
{
    return load_voice(disc, BFM_VOICE_ST02);
}

ForestVoice *bfm_inn_voice_load(const MusashiDiscMedia *disc)
{
    return load_voice(disc, BFM_VOICE_SHOPS01);
}

ForestVoice *bfm_library_voice_load(const MusashiDiscMedia *disc)
{
    return load_voice(disc, BFM_VOICE_OPEN);
}

int bfm_forest_voice_request(ForestVoice *v, unsigned bank, unsigned cue)
{
    BfmDiscVoiceCue selected;
    if (!v || !bfm_disc_voice_request(v->source, bank, cue, &selected)) return 0;
    v->status = BFM_FOREST_VOICE_PENDING;
    v->until_read = FIRST_READ_UPDATE;
    v->lead_in = selected.channel_offset * BFM_FOREST_VOICE_RATE / SECTORS_PER_SECOND;
    v->group_head_frames = selected.group_head_frames;
    v->remaining_ticks = selected.sound_ticks;
    v->frames_left = (size_t)((uint64_t)selected.sound_ticks * BFM_FOREST_VOICE_RATE * 1001 / 60000);
    return 1;
}

void bfm_forest_voice_step(ForestVoice *v)
{
    if (!v || v->status == BFM_FOREST_VOICE_IDLE) return;
    if (v->status == BFM_FOREST_VOICE_PENDING) {
        if (!v->until_read) {
            bfm_forest_voice_stop(v);
        } else if (!--v->until_read) {
            if (v->remaining_ticks && bfm_disc_voice_prime(v->source))
                v->status = BFM_FOREST_VOICE_PLAYING;
            else
                bfm_forest_voice_stop(v);
        }
    } else if (v->remaining_ticks <= 2) {
        /* Mute immediately, then retire the acknowledged pause on the next
         * service opportunity. The foreground sees a pending pause here. */
        v->status = BFM_FOREST_VOICE_PENDING;
        v->frames_left = 0;
    } else {
        v->remaining_ticks -= 2;
    }
}

unsigned bfm_forest_voice_status(const ForestVoice *v)
{
    return v ? v->status : BFM_FOREST_VOICE_IDLE;
}

int bfm_bedroom_voice_primed_frames(const ForestVoice *v, size_t *frames)
{
    if (!v || !frames || v->archive != BFM_VOICE_ST02 || v->status != BFM_FOREST_VOICE_PLAYING)
        return 0;
    *frames = v->group_head_frames;
    return 1;
}

size_t bfm_forest_voice_read(ForestVoice *v, int16_t *stereo, size_t frames)
{
    if (!v || !stereo || v->status != BFM_FOREST_VOICE_PLAYING) return 0;
    if (frames > v->frames_left) frames = v->frames_left;
    size_t silence = v->lead_in < frames ? v->lead_in : frames;
    /* Reading status is visible before the first matching channel sector. */
    memset(stereo, 0, silence * 2 * sizeof(*stereo));
    v->lead_in -= silence;
    size_t written = silence + bfm_disc_voice_read(v->source, stereo + 2 * silence, frames - silence);
    v->frames_left -= written;
    return written;
}

void bfm_forest_voice_stop(ForestVoice *v)
{
    if (!v) return;
    bfm_disc_voice_stop(v->source);
    v->status = BFM_FOREST_VOICE_IDLE;
    v->until_read = v->remaining_ticks = 0;
    v->frames_left = v->lead_in = 0;
}

void bfm_forest_voice_free(ForestVoice *v)
{
    if (!v) return;
    bfm_disc_voice_free(v->source);
    free(v);
}

const BfmForestMessageFrame *bfm_forest_voice_page(ForestVoice *voice, BfmForestMessage *page,
                                                   unsigned *generation, int confirm, int reveal)
{
    const BfmForestMessageFrame *frame = bfm_field_message_tick(
        page, bfm_forest_voice_status(voice), confirm, reveal);
    if (!frame) return NULL;
    if (frame->events & BFM_FOREST_VOICE_STOP) bfm_forest_voice_stop(voice);
    if (frame->events & BFM_FOREST_VOICE_REQUEST) {
        if (!bfm_forest_voice_request(voice, frame->voice_bank, frame->voice_cue)) return NULL;
        ++*generation;
    }
    return frame;
}
