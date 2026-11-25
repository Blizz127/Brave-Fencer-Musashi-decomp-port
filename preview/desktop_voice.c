/* SPDX-License-Identifier: MIT */
#include "desktop_voice.h"
#include "palace_voice.h"
#include "forest_voice.h"
#include <stdlib.h>

/* Rendering and reading the mounted image share the game thread. Keep enough
 * speech queued to cover a slow frame; cue changes discard this lookahead. */
enum { BLOCK_FRAMES = 1024, QUEUE_BLOCKS = 16 };
/* A mounted image can decode long before the original opening's drive was
 * ready. Model its startup separately from the delay to digital output. */
enum { MESSAGE_READY_FIELDS = 32, STREAM_CHANGE_FIELDS = 10, OUTPUT_LEAD_FIELDS = 12 };
struct DesktopVoice {
    PalaceVoice *source, *upcoming;
    int16_t first[BLOCK_FRAMES * BFM_PALACE_VOICE_CHANNELS];
    size_t first_frames;
    SDL_AudioDeviceID device;
    unsigned bank, cue, request_field, ready_delay, active_field;
    int selected, reading, message_ready, playing, reported;
    unsigned forest_generation;
    int silent;
    Uint64 decoded_frames;
    Uint32 submitted_before_start;
};

static void open_device(DesktopVoice *v)
{
    SDL_AudioSpec want = {0};
    want.freq = BFM_PALACE_VOICE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = BFM_PALACE_VOICE_CHANNELS;
    want.samples = 512;
    /* SDL converts this application format to the device format as necessary.
     * A new queued-output device starts paused. */
    const char *driver = SDL_GetCurrentAudioDriver();
    if (!driver || SDL_strcmp(driver, "dummy") == 0) return;
    v->device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (!v->device) SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Dialogue output unavailable: %s", SDL_GetError());
}

DesktopVoice *bfm_desktop_voice_open(const MusashiDiscMedia *disc)
{
    DesktopVoice *v = calloc(1, sizeof *v);
    if (!v) return NULL;
    v->source = bfm_palace_voice_load(disc);
    v->upcoming = bfm_palace_voice_load(disc);
    if (!v->source || !v->upcoming) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not open the mounted palace voice data");
        bfm_palace_voice_free(v->source);
        bfm_palace_voice_free(v->upcoming);
        free(v);
        return NULL;
    }
    open_device(v);
    return v;
}

static void clear_output(DesktopVoice *v)
{
    /* Invalidate the cue before clearing: a cleared queue never signals start. */
    v->selected = v->reading = v->reported = 0;
    v->message_ready = v->playing = 0;
    v->first_frames = 0;
    v->forest_generation = 0;
    v->decoded_frames = 0;
    v->submitted_before_start = 0;
    if (v->device) {
        SDL_PauseAudioDevice(v->device, 1);
        SDL_ClearQueuedAudio(v->device);
    }
}

void bfm_desktop_voice_stop(DesktopVoice *v)
{
    if (!v) return;
    clear_output(v);
    bfm_palace_voice_stop(v->source);
    bfm_palace_voice_stop(v->upcoming);
}

/* Keep the decoder and message clock alive when a device disappears. A new
 * output starts with the next cue, so already consumed words never repeat. */
static void lose_device(DesktopVoice *v)
{
    SDL_CloseAudioDevice(v->device);
    v->device = 0;
    v->silent = 1;
    v->playing = 0;
}

void bfm_desktop_voice_event(DesktopVoice *v, const SDL_AudioDeviceEvent *e)
{
    if (!v || !e || e->iscapture) return;
    if (e->type == SDL_AUDIODEVICEREMOVED && v->device && e->which == v->device) {
        lose_device(v);
    } else if (e->type == SDL_AUDIODEVICEADDED && !v->device) {
        open_device(v);
    }
}

/* The opening runs at 60000/1001 fields per second. A silent cue consumes
 * decoded stereo frames on that clock, even across an output-device change. */
static Sint64 queued_frames(const DesktopVoice *v, unsigned field)
{
    if (!v->silent && v->device)
        return SDL_GetQueuedAudioSize(v->device) / (BFM_PALACE_VOICE_CHANNELS * sizeof(int16_t));
    unsigned age = field - v->active_field;
    Uint64 elapsed = (Uint64)age * BFM_PALACE_VOICE_RATE * 1001 / 60000;
    return (Sint64)v->decoded_frames - (Sint64)elapsed;
}

int bfm_desktop_voice_request(DesktopVoice *v, unsigned bank, unsigned cue, unsigned field)
{
    if (!v || (v->selected && v->bank == bank && v->cue == cue)) return 0;
    /* Interrupting an active XA stream takes longer than starting from idle. */
    int interrupted = v->selected && (v->reading || queued_frames(v, field) > 0);
    if (!bfm_palace_voice_request(v->upcoming, bank, cue)) return 0;
    size_t frames = bfm_palace_voice_read(v->upcoming, v->first, BLOCK_FRAMES);
    if (!frames) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not decode mounted palace voice %u/%u", bank, cue);
        return 0;
    }
    v->first_frames = frames;
    v->message_ready = v->reported = 0;
    v->bank = bank;
    v->cue = cue;
    v->request_field = field;
    v->ready_delay = MESSAGE_READY_FIELDS + (interrupted ? STREAM_CHANGE_FIELDS : 0);
    v->selected = 1;
    return 1;
}

int bfm_desktop_voice_pump(DesktopVoice *v, unsigned field, unsigned *bank, unsigned *cue)
{
    if (!v || !bank || !cue || !v->selected) return 0;
    const Uint32 frame_bytes = BFM_PALACE_VOICE_CHANNELS * sizeof(int16_t);
    unsigned age = field - v->request_field;
    Sint64 queued = queued_frames(v, field);
    int events = 0;
    if (!v->message_ready && age >= v->ready_delay) {
        v->message_ready = 1;
        events |= BFM_VOICE_MESSAGE_READY;
    }
    int start = v->first_frames && age >= v->ready_delay + OUTPUT_LEAD_FIELDS;
    if (start) {
        /* Continue feeding the old decoder until the new voice starts. A
         * queue cleared on request would cut off a word in mid-sentence. */
        PalaceVoice *old = v->source;
        v->source = v->upcoming;
        v->upcoming = old;
        v->active_field = field;
        v->silent = !v->device;
        v->playing = 0;
        v->decoded_frames = v->first_frames;
        v->submitted_before_start = 0;
        v->reading = v->first_frames == BLOCK_FRAMES;
        if (v->device) {
            SDL_PauseAudioDevice(v->device, 1);
            SDL_ClearQueuedAudio(v->device);
            Uint32 bytes = (Uint32)v->first_frames * frame_bytes;
            if (SDL_QueueAudio(v->device, v->first, bytes) < 0)
                lose_device(v);
            else
                v->submitted_before_start = bytes;
        }
        queued = (Sint64)v->first_frames;
        v->first_frames = 0;
    } else if (!v->first_frames && v->playing && !v->reported &&
               queued < v->submitted_before_start / frame_bytes &&
               SDL_GetAudioDeviceStatus(v->device) == SDL_AUDIO_PLAYING) {
        events |= BFM_VOICE_OUTPUT_STARTED;
        v->reported = 1;
    }
    while (v->reading && queued < BLOCK_FRAMES * QUEUE_BLOCKS) {
        int16_t pcm[BLOCK_FRAMES * BFM_PALACE_VOICE_CHANNELS];
        size_t frames = bfm_palace_voice_read(v->source, pcm, BLOCK_FRAMES);
        if (frames < BLOCK_FRAMES) v->reading = 0;
        if (!frames) break;
        Uint32 bytes = (Uint32)frames * frame_bytes;
        if (!v->silent && SDL_QueueAudio(v->device, pcm, bytes) < 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Could not queue palace voice: %s", SDL_GetError());
            lose_device(v);
            queued = queued_frames(v, field);
        }
        if (!v->silent && !v->first_frames && !v->reported) v->submitted_before_start += bytes;
        v->decoded_frames += frames;
        queued += (Sint64)frames;
    }
    if (start && !v->silent) {
        SDL_PauseAudioDevice(v->device, 0);
        v->playing = 1;
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
    /* A new request spends several updates seeking. Let the queued end of
     * the previous line play until the incoming stream is ready. */
    if (v->forest_generation != generation) {
        clear_output(v);
        v->forest_generation = generation;
    }
    const Uint32 frame_bytes = 2 * sizeof(int16_t);
    Uint32 queued = SDL_GetQueuedAudioSize(v->device);
    while (queued < BLOCK_FRAMES * QUEUE_BLOCKS * frame_bytes) {
        int16_t pcm[BLOCK_FRAMES * 2];
        size_t frames = bfm_forest_voice_read(source, pcm, BLOCK_FRAMES);
        if (!frames) break;
        Uint32 bytes = (Uint32)frames * frame_bytes;
        if (SDL_QueueAudio(v->device, pcm, bytes) < 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Could not queue forest voice: %s", SDL_GetError());
            break;
        }
        queued += bytes;
    }
    if (queued && SDL_GetAudioDeviceStatus(v->device) == SDL_AUDIO_PAUSED)
        SDL_PauseAudioDevice(v->device, 0);
}

void bfm_desktop_voice_free(DesktopVoice *v)
{
    if (!v) return;
    bfm_desktop_voice_stop(v);
    if (v->device) SDL_CloseAudioDevice(v->device);
    bfm_palace_voice_free(v->source);
    bfm_palace_voice_free(v->upcoming);
    free(v);
}
