/* SPDX-License-Identifier: MIT */
#include "desktop_voice.h"
#include "palace_voice.h"
#include <stdlib.h>

/* Rendering and reading the mounted image share the game thread. Keep enough
 * speech queued to cover a slow frame; cue changes discard this lookahead. */
enum { BLOCK_FRAMES = 1024, QUEUE_BLOCKS = 16 };
struct DesktopVoice {
    PalaceVoice *source;
    SDL_AudioDeviceID device;
    unsigned bank, cue;
    int selected, reading, reported;
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
    if (!v->source) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Could not open the mounted palace voice data");
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
}

void bfm_desktop_voice_event(DesktopVoice *v, const SDL_AudioDeviceEvent *e)
{
    if (!v || !e || e->iscapture) return;
    if (e->type == SDL_AUDIODEVICEREMOVED && v->device && e->which == v->device) {
        bfm_desktop_voice_stop(v);
        SDL_CloseAudioDevice(v->device);
        v->device = 0;
    } else if (e->type == SDL_AUDIODEVICEADDED && !v->device) {
        open_device(v);
    }
}

int bfm_desktop_voice_request(DesktopVoice *v, unsigned bank, unsigned cue)
{
    if (!v || !v->device || (v->selected && v->bank == bank && v->cue == cue)) return 0;
    if (!bfm_palace_voice_request(v->source, bank, cue)) return 0;
    clear_output(v);
    v->bank = bank;
    v->cue = cue;
    v->selected = 1;
    v->reading = 1;
    return 1;
}

int bfm_desktop_voice_pump(DesktopVoice *v, unsigned *bank, unsigned *cue)
{
    if (!v || !bank || !cue || !v->device || !v->selected) return 0;
    const Uint32 frame_bytes = BFM_PALACE_VOICE_CHANNELS * sizeof(int16_t);
    Uint32 queued = SDL_GetQueuedAudioSize(v->device);
    int started = !v->reported && queued < v->submitted_before_start &&
                  SDL_GetAudioDeviceStatus(v->device) == SDL_AUDIO_PLAYING;
    if (started) {
        *bank = v->bank;
        *cue = v->cue;
        v->reported = 1;
    }
    int first_buffer = v->submitted_before_start == 0 && !v->reported;
    while (v->reading && queued < BLOCK_FRAMES * QUEUE_BLOCKS * frame_bytes) {
        int16_t pcm[BLOCK_FRAMES * BFM_PALACE_VOICE_CHANNELS];
        size_t frames = bfm_palace_voice_read(v->source, pcm, BLOCK_FRAMES);
        if (frames < BLOCK_FRAMES) v->reading = 0;
        if (!frames) break;
        Uint32 bytes = (Uint32)frames * frame_bytes;
        if (SDL_QueueAudio(v->device, pcm, bytes) < 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Could not queue palace voice: %s", SDL_GetError());
            bfm_desktop_voice_stop(v);
            break;
        }
        if (!v->reported) v->submitted_before_start += bytes;
        queued += bytes;
    }
    if (first_buffer && v->submitted_before_start)
        SDL_PauseAudioDevice(v->device, 0);
    return started;
}

void bfm_desktop_voice_free(DesktopVoice *v)
{
    if (!v) return;
    bfm_desktop_voice_stop(v);
    if (v->device) SDL_CloseAudioDevice(v->device);
    bfm_palace_voice_free(v->source);
    free(v);
}
