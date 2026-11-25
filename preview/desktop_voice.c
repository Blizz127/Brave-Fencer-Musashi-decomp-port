/* SPDX-License-Identifier: MIT */
#include "desktop_voice.h"
#include "palace_voice.h"
#include "forest_voice.h"
#include <stdlib.h>
#include <string.h>

/* Model the mounted drive's startup separately from its digital output. */
enum { MESSAGE_READY_FIELDS = 32, STREAM_CHANGE_FIELDS = 10, OUTPUT_LEAD_FIELDS = 12 };
enum { FRAME_BYTES = BFM_PALACE_VOICE_CHANNELS * sizeof(int16_t) };
typedef struct {
    size_t frames, signal_end;
    int16_t pcm[];
} VoiceClip;
typedef struct {
    VoiceClip *clip;
    size_t cursor, end;
} Playback;

struct DesktopVoice {
    PalaceVoice *decoder;
    VoiceClip *pending;
    SDL_AudioDeviceID device;
    unsigned bank, cue, request_field, ready_delay, active_field, forest_generation;
    int selected, message_ready, playing, reported;
    /* Only these spans are visible to SDL. Its device lock guards their
     * ownership and cursors. The callback never allocates or touches the disc. */
    Playback outgoing, active;
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

static size_t copy_frames(Playback *p, Uint8 *out, size_t wanted)
{
    size_t count = p->end - p->cursor;
    if (count > wanted) count = wanted;
    if (count) {
        memcpy(out, p->clip->pcm + p->cursor * BFM_PALACE_VOICE_CHANNELS, count * FRAME_BYTES);
        p->cursor += count;
    }
    return count;
}

static void SDLCALL render_voice(void *userdata, Uint8 *stream, int bytes)
{
    DesktopVoice *v = userdata;
    size_t frames = (size_t)bytes / FRAME_BYTES;
    size_t n = copy_frames(&v->outgoing, stream, frames);
    n += copy_frames(&v->active, stream + n * FRAME_BYTES, frames - n);
    memset(stream + n * FRAME_BYTES, 0, (size_t)bytes - n * FRAME_BYTES);
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
    open_device(v);
    return v;
}

void bfm_desktop_voice_stop(DesktopVoice *v)
{
    if (!v) return;
    /* Pausing waits for the in-flight callback and keeps it suspended until
     * a new digital start. */
    if (v->device) SDL_PauseAudioDevice(v->device, 1);
    free(v->outgoing.clip);
    free(v->active.clip);
    free(v->pending);
    v->outgoing = (Playback){0};
    v->active = (Playback){0};
    v->pending = NULL;
    v->forest_generation = 0;
    v->selected = v->message_ready = v->playing = v->reported = 0;
    bfm_palace_voice_stop(v->decoder);
}

void bfm_desktop_voice_event(DesktopVoice *v, const SDL_AudioDeviceEvent *e)
{
    if (!v || !e || e->iscapture) return;
    if (e->type == SDL_AUDIODEVICEREMOVED && v->device && e->which == v->device) {
        SDL_CloseAudioDevice(v->device);
        v->device = 0;
        v->playing = 0;
    } else if (e->type == SDL_AUDIODEVICEADDED && !v->device) {
        /* A replacement device stays paused until the next cue. */
        open_device(v);
    }
}

static Uint64 elapsed_frames(unsigned fields)
{
    return (Uint64)fields * BFM_PALACE_VOICE_RATE * 1001 / 60000;
}

int bfm_desktop_voice_request(DesktopVoice *v, unsigned bank, unsigned cue, unsigned field)
{
    if (!v || (v->selected && v->bank == bank && v->cue == cue)) return 0;
    /* Disc readiness follows the scene clock even if the host device runs
     * slowly, disappears, or is completely silent. */
    int interrupted = !v->forest_generation && v->active.clip &&
        elapsed_frames(field - v->active_field) < v->active.end;
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

static void start_clip(DesktopVoice *v, unsigned due, unsigned forest_generation)
{
    if (v->device) SDL_LockAudioDevice(v->device);
    VoiceClip *retired = v->outgoing.clip;
    VoiceClip *old = v->active.clip;
    v->outgoing = (Playback){0};
    /* If the outgoing line fits its scene interval, retain any audible tail
     * the device has yet to consume. Truly interrupted lines switch at once. */
    if (!forest_generation && !v->forest_generation && old && v->playing &&
        v->active.cursor < old->signal_end &&
        old->signal_end <= elapsed_frames(due - v->active_field)) {
        v->outgoing = (Playback){old, v->active.cursor, old->signal_end};
        old = NULL;
    }
    v->active = (Playback){v->pending, 0, v->pending->frames};
    v->pending = NULL;
    v->active_field = due;
    v->forest_generation = forest_generation;
    v->playing = v->device != 0;
    if (v->device) {
        SDL_UnlockAudioDevice(v->device);
        SDL_PauseAudioDevice(v->device, 0);
    }
    free(retired);
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
        start_clip(v, bfm_desktop_voice_ready_field(v) + OUTPUT_LEAD_FIELDS, 0);
    } else if (!v->pending && v->playing && !v->reported) {
        SDL_LockAudioDevice(v->device);
        int started = v->active.cursor != 0;
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

void bfm_desktop_voice_forest(DesktopVoice *v, ForestVoice *source, unsigned generation)
{
    if (!v || !v->device || !source || !generation ||
        bfm_forest_voice_status(source) != BFM_FOREST_VOICE_PLAYING)
        return;
    if (generation == v->forest_generation) {
        if (!v->playing) {
            /* Resume the unconsumed part on a replacement output device. */
            v->playing = 1;
            SDL_PauseAudioDevice(v->device, 0);
        }
        return;
    }
    /* The forest transport owns the seek, channel lead-in and sound deadline.
     * Keep the outgoing line until the new generation really starts playing. */
    VoiceClip *next = read_clip(NULL, source);
    if (!next) return;
    free(v->pending);
    v->pending = next;
    v->selected = v->message_ready = v->reported = 0;
    start_clip(v, 0, generation);
}

void bfm_desktop_voice_free(DesktopVoice *v)
{
    if (!v) return;
    bfm_desktop_voice_stop(v);
    if (v->device) SDL_CloseAudioDevice(v->device);
    bfm_palace_voice_free(v->decoder);
    free(v);
}
