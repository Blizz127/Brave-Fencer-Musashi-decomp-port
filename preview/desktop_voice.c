/* SPDX-License-Identifier: MIT */
#include "desktop_voice.h"
#include "palace_voice.h"
#include "forest_voice.h"
#include "forest_score.h"
#include "bedroom_effects.h"
#include "palace_effects.h"
#include "scene_views.h"
#include <stdlib.h>
#include <string.h>

/* Model the mounted drive's startup separately from its digital output. */
enum { MESSAGE_READY_FIELDS = 32, STREAM_CHANGE_FIELDS = 10, OUTPUT_LEAD_FIELDS = 12 };
enum { FRAME_BYTES = BFM_PALACE_VOICE_CHANNELS * sizeof(int16_t) };
typedef struct VoiceClip {
    size_t frames, signal_end, primed_frames;
    int staged_start;
    size_t cursor, end;
    Uint64 continuous_from;
    struct VoiceClip *next;
    int16_t pcm[];
} VoiceClip;

struct DesktopVoice {
    PalaceVoice *decoder;
    const ForestVoice *active_source;
    BfmBedroomEffects *room_effects, *forest_swing;
    BfmPalaceEffects *effects;
    BfmForestScore *forest_score, *room_score, *fading_score;
    Uint64 forest_score_start;
    int forest_score_requested, forest_score_waiting;
    Uint64 opening_start, rendered_frames;
    enum { COURT_IDLE, COURT_PLAYING, COURT_FINISHING } court;
    VoiceClip *pending;
    SDL_AudioDeviceID device;
    unsigned bank, cue, request_field, ready_delay, active_field, forest_generation;
    int selected, message_ready, playing, reported;
    /* Only these spans are visible to SDL. Its device lock guards their
     * ownership and cursors. The callback never allocates or touches the disc. */
    VoiceClip *outgoing, *outgoing_tail, *retired;
    VoiceClip *active;
};

static VoiceClip *read_clip(PalaceVoice *palace, ForestVoice *forest)
{
    size_t capacity = 8192, used = 0;
    VoiceClip *clip = NULL;
    const size_t max_frames = (SIZE_MAX - sizeof *clip) / FRAME_BYTES;
    while (capacity <= max_frames) {
        VoiceClip *grown = realloc(clip, sizeof *clip + capacity * FRAME_BYTES);
        if (!grown) break;
        clip = grown;
        size_t available = capacity - used;
        int16_t *out = clip->pcm + used * BFM_PALACE_VOICE_CHANNELS;
        size_t read = forest ? bfm_forest_voice_read(forest, out, available) :
                               bfm_palace_voice_read(palace, out, available);
        used += read;
        if (read < available) {
            if (!used) break;
            clip->frames = used;
            clip->primed_frames = 0;
            clip->staged_start = bfm_bedroom_voice_primed_frames(forest, &clip->primed_frames);
            while (used && !clip->pcm[2 * used - 2] && !clip->pcm[2 * used - 1]) --used;
            clip->signal_end = used;
            return clip;
        }
        if (capacity > max_frames / 2) break;
        capacity *= 2;
    }
    free(clip);
    return NULL;
}

static size_t copy_until(VoiceClip *p, Uint8 *out, size_t wanted, size_t end)
{
    size_t count = end - p->cursor;
    if (count > wanted) count = wanted;
    if (count) {
        memcpy(out, p->pcm + p->cursor * BFM_PALACE_VOICE_CHANNELS, count * FRAME_BYTES);
        p->cursor += count;
    }
    return count;
}

static size_t copy_frames(VoiceClip *p, Uint8 *out, size_t wanted, Uint64 start)
{
    if (!p || p->cursor == p->end) return 0;
    size_t written = 0;
    if (start < p->continuous_from) {
        size_t primed = p->primed_frames < p->end ? p->primed_frames : p->end;
        if (p->cursor < primed)
            written = copy_until(p, out, wanted, primed);
        if (p->cursor == p->end || written == wanted) return written;
        if (start + written < p->continuous_from) {
            Uint64 wait = p->continuous_from - start - written;
            size_t silence = wait < wanted - written ? (size_t)wait : wanted - written;
            memset(out + written * FRAME_BYTES, 0, silence * FRAME_BYTES);
            written += silence;
        }
    }
    return written + copy_until(p, out + written * FRAME_BYTES, wanted - written, p->end);
}

static size_t render_speech(DesktopVoice *v, Uint8 *stream, size_t frames, Uint64 start)
{
    /* The reopened device can play room music before the scene confirms
     * ownership of the old speech. Leave its cursor still until it does. */
    if (v->active && v->active->staged_start && !v->playing) return 0;
    size_t n = 0;
    while (v->outgoing && n < frames) {
        VoiceClip *first = v->outgoing;
        n += copy_frames(first, stream + n * FRAME_BYTES, frames - n, start + n);
        if (first->cursor < first->end) break;
        v->outgoing = first->next;
        if (!v->outgoing) v->outgoing_tail = NULL;
        first->next = v->retired;
        v->retired = first;
    }
    return n + copy_frames(v->active, stream + n * FRAME_BYTES, frames - n, start + n);
}

static void SDLCALL render_voice(void *userdata, Uint8 *stream, int bytes)
{
    DesktopVoice *v = userdata;
    size_t frames = (size_t)bytes / FRAME_BYTES;
    size_t n = render_speech(v, stream, frames, v->rendered_frames);
    v->rendered_frames += frames;
    memset(stream + n * FRAME_BYTES, 0, (size_t)bytes - n * FRAME_BYTES);
    if (v->fading_score && bfm_forest_score_mix(v->fading_score, (int16_t *)(void *)stream, frames) < frames)
        v->fading_score = NULL;
    if (v->room_score) bfm_forest_score_mix(v->room_score, (int16_t *)(void *)stream, frames);
    bfm_bedroom_effects_mix(v->room_effects, (int16_t *)(void *)stream, frames);
    if (v->court != COURT_IDLE)
        bfm_palace_effects_mix(v->effects, SDL_GetTicks64() - v->opening_start,
                              (int16_t *)(void *)stream, frames);

    if (v->forest_score_requested && v->forest_score) {
        size_t lead = 0;
        if (v->forest_score_waiting) {
            Uint64 now = SDL_GetTicks64();
            if (now < v->forest_score_start) {
                Uint64 wait_ms = v->forest_score_start - now;
                if (wait_ms >= ((Uint64)frames * 1000 + BFM_PALACE_VOICE_RATE - 1) /
                               BFM_PALACE_VOICE_RATE) return;
                lead = (size_t)((wait_ms * BFM_PALACE_VOICE_RATE + 999) / 1000);
                if (lead >= frames) return;
            }
            v->forest_score_waiting = 0;
        }
        bfm_forest_score_mix(v->forest_score,
                             (int16_t *)(void *)stream + lead * BFM_PALACE_VOICE_CHANNELS,
                             frames - lead);
    }
}

static void open_device(DesktopVoice *v)
{
    SDL_AudioSpec want = {0};
    want.freq = BFM_PALACE_VOICE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = BFM_PALACE_VOICE_CHANNELS;
    want.samples = 512;
    want.callback = render_voice;
    want.userdata = v;
    const char *driver = SDL_GetCurrentAudioDriver();
    if (!driver || SDL_strcmp(driver, "dummy") == 0) return;
    v->device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (!v->device) SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Dialogue output unavailable: %s", SDL_GetError());
}

DesktopVoice *bfm_desktop_voice_open(const MusashiDiscMedia *disc)
{
    DesktopVoice *v = calloc(1, sizeof *v);
    if (!v) return NULL;
    v->decoder = bfm_palace_voice_load(disc);
    if (!v->decoder) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not open the mounted palace voice data");
        free(v);
        return NULL;
    }
    v->effects = bfm_palace_effects_load(disc);
    open_device(v);
    return v;
}

void bfm_desktop_voice_opening(DesktopVoice *v, Uint64 start)
{
    if (!v) return;
    if (v->device) SDL_LockAudioDevice(v->device);
    bfm_palace_effects_reset(v->effects);
    v->opening_start = start;
    v->court = COURT_PLAYING;
    if (v->device) SDL_UnlockAudioDevice(v->device);
}

/* Clips are allocated and freed on the game thread. The callback only
 * removes consumed clips from the queue and links them for retirement. */
static void free_clips(VoiceClip *first)
{
    while (first) {
        VoiceClip *next = first->next;
        free(first);
        first = next;
    }
}

static void stop_voice(DesktopVoice *v, int leaving_room)
{
    if (!v) return;
    /* Circle cancels speech under the device lock while the room's
     * score and effects continue to play. */
    if (v->device) SDL_LockAudioDevice(v->device);
    free_clips(v->outgoing);
    free_clips(v->retired);
    free(v->active);
    free(v->pending);
    v->outgoing = v->outgoing_tail = v->retired = NULL;
    v->active = NULL;
    v->pending = NULL;
    if (leaving_room) {
        v->room_score = v->fading_score = NULL;
        v->room_effects = NULL;
        bfm_bedroom_effects_free(v->forest_swing);
        v->forest_swing = NULL;
    }
    v->court = COURT_IDLE;
    v->forest_generation = 0;
    v->active_source = NULL;
    v->forest_score_requested = v->forest_score_waiting = 0;
    v->selected = v->message_ready = v->playing = v->reported = 0;
    int room_audio = v->room_score || v->fading_score || v->room_effects;
    if (v->device) {
        SDL_UnlockAudioDevice(v->device);
        if (!room_audio) SDL_PauseAudioDevice(v->device, 1);
    }
    bfm_palace_voice_stop(v->decoder);
}

void bfm_desktop_voice_room_skip(DesktopVoice *v)
{
    stop_voice(v, 0);
}

void bfm_desktop_voice_stop(DesktopVoice *v)
{
    stop_voice(v, 1);
}

void bfm_desktop_voice_room_effect(DesktopVoice *v, BfmBedroomEffects *effects, unsigned command)
{
    if (!v || !effects || !command) return;
    if (v->device) SDL_LockAudioDevice(v->device);
    v->room_effects = effects;
    bfm_bedroom_effects_start(effects, command);
    if (v->device) {
        SDL_UnlockAudioDevice(v->device);
        SDL_PauseAudioDevice(v->device, 0);
    }
}

void bfm_desktop_voice_forest_swing(DesktopVoice *v)
{
    if (v) bfm_desktop_voice_room_effect(v, v->forest_swing, BFM_FOREST_SWING_SOUND);
}

void bfm_desktop_voice_forest_hit(DesktopVoice *v)
{
    if (v) bfm_desktop_voice_room_effect(v, v->forest_swing, BFM_FOREST_HIT_SOUND);
}

void bfm_desktop_voice_room_score(DesktopVoice *v, BfmForestScore *score)
{
    if (!v || v->room_score == score) return;
    if (v->device) SDL_LockAudioDevice(v->device);
    if (v->room_score) {
        v->fading_score = v->room_score;
        bfm_castle_score_fade(v->fading_score);
    }
    v->room_score = score;
    if (v->device) SDL_UnlockAudioDevice(v->device);
    if (v->device && score) SDL_PauseAudioDevice(v->device, 0);
}

void bfm_desktop_voice_forest_score(DesktopVoice *v, const MusashiDiscMedia *disc, Uint64 start)
{
    if (!v || !disc || v->forest_score_requested) return;
    /* The card has faded and the prior court sound is silent. Prepare
     * the mounted score with the device paused, ahead of its first note. */
    bfm_desktop_voice_stop(v);
    v->forest_swing = bfm_forest_swing_sound_load(disc);
    if (!v->forest_score)
        v->forest_score = bfm_forest_score_load(disc, BFM_PALACE_VOICE_RATE);
    v->forest_score_requested = 1;
    if (!v->forest_score) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Could not read the mounted Giant Forest score.");
        return;
    }
    bfm_forest_score_rewind(v->forest_score);
    v->forest_score_start = start;
    v->forest_score_waiting = 1;
    if (v->device) SDL_PauseAudioDevice(v->device, 0);
}

void bfm_desktop_voice_event(DesktopVoice *v, const SDL_AudioDeviceEvent *e)
{
    if (!v || !e || e->iscapture) return;
    if (e->type == SDL_AUDIODEVICEREMOVED && v->device && e->which == v->device) {
        SDL_CloseAudioDevice(v->device);
        v->device = 0;
        v->playing = 0;
    } else if (e->type == SDL_AUDIODEVICEADDED && !v->device) {
        open_device(v);
        int room_audio = v->room_effects || v->room_score || v->fading_score;
        if (v->device && (room_audio || (v->forest_score_requested && v->forest_score))) {
            SDL_PauseAudioDevice(v->device, 0);
        } else if (v->device && v->court == COURT_PLAYING && v->effects) {
            bfm_palace_effects_reset(v->effects);
            SDL_PauseAudioDevice(v->device, 0);
        }
    }
}

static Uint64 elapsed_frames(unsigned fields)
{
    return (Uint64)fields * BFM_PALACE_VOICE_RATE * 1001 / 60000;
}

/* Each active track already has its scene-bounded sample count. Let the
 * device consume it after the final pose; a return to title still stops it. */
void bfm_desktop_voice_finish(DesktopVoice *v)
{
    if (!v) return;
    if (!v->device) {
        bfm_desktop_voice_stop(v);
        v->court = COURT_FINISHING;
        return;
    }
    SDL_LockAudioDevice(v->device);
    v->court = COURT_FINISHING;
    SDL_UnlockAudioDevice(v->device);
    free(v->pending);
    v->pending = NULL;
    v->selected = 0;
}

int bfm_desktop_voice_request(DesktopVoice *v, unsigned bank, unsigned cue, unsigned field)
{
    if (!v || v->court == COURT_FINISHING || (v->selected && v->bank == bank && v->cue == cue)) return 0;
    /* Disc readiness follows the scene clock even if the host device runs
     * slowly, disappears, or is completely silent. */
    int interrupted = !v->forest_generation && v->active &&
        elapsed_frames(field - v->active_field) < v->active->end;
    if (!bfm_palace_voice_request(v->decoder, bank, cue)) return 0;
    VoiceClip *next = read_clip(v->decoder, NULL);
    if (!next) return 0;
    free(v->pending);
    v->pending = next;
    v->message_ready = v->reported = 0;
    v->bank = bank;
    v->cue = cue;
    v->request_field = field;
    v->ready_delay = MESSAGE_READY_FIELDS + (interrupted ? STREAM_CHANGE_FIELDS : 0);
    v->selected = 1;
    return 1;
}

unsigned bfm_desktop_voice_ready_field(const DesktopVoice *v)
{
    return v && v->selected ? v->request_field + v->ready_delay : 0;
}

static void start_clip(DesktopVoice *v, unsigned due, unsigned forest_generation,
                       const ForestVoice *source)
{
    if (v->device) SDL_LockAudioDevice(v->device);
    VoiceClip *old = v->active;
    size_t keep = 0;
    if (old && v->playing) {
        if (!forest_generation && !v->forest_generation &&
            old->signal_end <= elapsed_frames(due - v->active_field))
            keep = old->signal_end;
        else if (forest_generation && v->active_source == source && v->active_field &&
                 due > v->active_field && old->frames <= elapsed_frames(due - v->active_field))
            keep = old->frames;
    }
    /* Only a naturally elapsed bedroom-to-bedroom transition keeps all
     * previously owed spans. An early replacement discards that backlog. */
    if (v->outgoing && !(keep && old->staged_start && v->pending->staged_start)) {
        v->outgoing_tail->next = v->retired;
        v->retired = v->outgoing;
        v->outgoing = v->outgoing_tail = NULL;
    }
    if (old && old->cursor < keep) {
        old->end = keep;
        old->next = NULL;
        if (v->outgoing_tail) v->outgoing_tail->next = old;
        else v->outgoing = old;
        v->outgoing_tail = old;
        old = NULL;
    }
    VoiceClip *retired = v->retired;
    v->retired = NULL;
    size_t frames = v->pending->frames;
    if (!forest_generation) {
        /* Limit the clip before exposing it to a faster audio callback. */
        Uint64 left = due < BFM_OPENING_LAST_FIELD ? elapsed_frames(BFM_OPENING_LAST_FIELD - due) : 0;
        if (left < frames) frames = (size_t)left;
    }
    v->active = v->pending;
    v->active->cursor = 0;
    v->active->end = frames;
    v->active->continuous_from = 0;
    v->active->next = NULL;
    if (v->pending->staged_start) {
        /* The ready group-head sector can sound immediately. Subsequent
         * ST02 sectors follow the measured drive-to-output delay. Time
         * spent finishing the preceding line also counts toward readiness. */
        v->active->continuous_from = v->rendered_frames + v->pending->primed_frames +
                                    elapsed_frames(OUTPUT_LEAD_FIELDS);
    }
    v->pending = NULL;
    v->active_field = due;
    v->forest_generation = forest_generation;
    v->active_source = source;
    v->playing = v->device != 0;
    if (v->device) {
        SDL_UnlockAudioDevice(v->device);
        SDL_PauseAudioDevice(v->device, 0);
    }
    free_clips(retired);
    free(old);
}

int bfm_desktop_voice_pump(DesktopVoice *v, unsigned field, unsigned *bank, unsigned *cue)
{
    if (!v || !bank || !cue || !v->selected) return 0;
    unsigned age = field - v->request_field;
    int events = 0;
    if (!v->message_ready && age >= v->ready_delay) {
        v->message_ready = 1;
        events |= BFM_VOICE_MESSAGE_READY;
    }
    if (v->pending && age >= v->ready_delay + OUTPUT_LEAD_FIELDS) {
        start_clip(v, bfm_desktop_voice_ready_field(v) + OUTPUT_LEAD_FIELDS, 0, NULL);
    } else if (!v->pending && v->playing && !v->reported) {
        SDL_LockAudioDevice(v->device);
        int started = v->active->cursor != 0;
        SDL_UnlockAudioDevice(v->device);
        if (started && SDL_GetAudioDeviceStatus(v->device) == SDL_AUDIO_PLAYING) {
            v->reported = 1;
            events |= BFM_VOICE_OUTPUT_STARTED;
        }
    }
    if (events) {
        *bank = v->bank;
        *cue = v->cue;
    }
    return events;
}

void bfm_desktop_voice_forest(DesktopVoice *v, ForestVoice *source, unsigned generation, unsigned room_field)
{
    if (!v || !v->device || !source || !generation) return;
    unsigned status = bfm_forest_voice_status(source);
    if (v->active_source == source && generation == v->forest_generation) {
        if (!v->playing) {
            /* A replacement device can resume an accepted bedroom line
             * after the scene clock has retired the disc request. */
            SDL_LockAudioDevice(v->device);
            int owed = v->active && v->active->staged_start &&
                      (v->outgoing || v->active->cursor < v->active->end);
            int resume = owed || status == BFM_FOREST_VOICE_PLAYING;
            if (resume) v->playing = 1;
            SDL_UnlockAudioDevice(v->device);
            if (resume) SDL_PauseAudioDevice(v->device, 0);
        }
        return;
    }
    if (status != BFM_FOREST_VOICE_PLAYING) return;
    /* The forest transport owns the seek, channel lead-in and sound deadline.
     * Keep the outgoing line until the new generation really starts playing. */
    VoiceClip *next = read_clip(NULL, source);
    if (!next) return;
    free(v->pending);
    v->pending = next;
    v->selected = v->message_ready = v->reported = 0;
    start_clip(v, room_field, generation, source);
}

void bfm_desktop_voice_free(DesktopVoice *v)
{
    if (!v) return;
    bfm_desktop_voice_stop(v);
    if (v->device) SDL_CloseAudioDevice(v->device);
    bfm_forest_score_free(v->forest_score);
    bfm_palace_effects_free(v->effects);
    bfm_palace_voice_free(v->decoder);
    free(v);
}
