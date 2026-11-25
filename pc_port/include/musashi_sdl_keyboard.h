#ifndef MUSASHI_SDL_KEYBOARD_H
#define MUSASHI_SDL_KEYBOARD_H

#include <SDL.h>
#include <stdint.h>
#include <string.h>

/* Button edges alternate, so a count per button retains tap/release order
 * without allocating event records. All masks here are active-high. */
typedef struct MusashiSdlKeyboard {
    uint16_t binding[SDL_NUM_SCANCODES];
    Uint8 down[SDL_NUM_SCANCODES];
    uint32_t pending[16];
    uint16_t held, delivered, preview;
    Uint32 window_id;
    SDL_SpinLock lock;
    int watching, unfocused, suppressed;
} MusashiSdlKeyboard;

static inline uint16_t musashi_sdl_keyboard_held(const MusashiSdlKeyboard *k) {
    uint16_t held = 0;
    for (unsigned i = 0; i < SDL_NUM_SCANCODES; ++i)
        if (k->down[i]) held |= k->binding[i];
    return held;
}

static inline void musashi_sdl_keyboard_flush(MusashiSdlKeyboard *k) {
    memset(k->pending, 0, sizeof k->pending);
    k->held = k->delivered = k->preview = 0;
}

static inline int SDLCALL musashi_sdl_keyboard_watch(void *userdata, SDL_Event *e) {
    MusashiSdlKeyboard *k = userdata;
    if (e->type == SDL_WINDOWEVENT) {
        if ((k->window_id && e->window.windowID != k->window_id) ||
            (e->window.event != SDL_WINDOWEVENT_FOCUS_LOST &&
             e->window.event != SDL_WINDOWEVENT_FOCUS_GAINED)) return 0;
        SDL_AtomicLock(&k->lock);
        k->unfocused = e->window.event == SDL_WINDOWEVENT_FOCUS_LOST;
        if (k->unfocused) {
            memset(k->down, 0, sizeof k->down);
            musashi_sdl_keyboard_flush(k);
        }
        SDL_AtomicUnlock(&k->lock);
        return 0;
    }
    if ((e->type != SDL_KEYDOWN && e->type != SDL_KEYUP) || e->key.repeat ||
        (k->window_id && e->key.windowID != k->window_id)) return 0;
    unsigned sc = (unsigned)e->key.keysym.scancode;
    if (sc >= SDL_NUM_SCANCODES) return 0;
    SDL_AtomicLock(&k->lock);
    Uint8 down = e->type == SDL_KEYDOWN;
    if (k->suppressed && !down) k->down[sc] = 0;
    if (!k->suppressed && !k->unfocused && down != k->down[sc]) {
        k->down[sc] = down;
        uint16_t held = k->held | k->binding[sc];
        if (!down && k->binding[sc]) {
            /* Multiple scancodes can hold the same logical button. */
            held = musashi_sdl_keyboard_held(k);
        }
        uint16_t changed = held ^ k->held;
        for (unsigned bit = 0; bit < 16; ++bit) {
            if (changed & (1u << bit)) {
                /* Drop the oldest pair on saturation, retaining final parity. */
                if (k->pending[bit] == UINT32_MAX) k->pending[bit] -= 2;
                ++k->pending[bit];
            }
        }
        k->held = held;
    }
    SDL_AtomicUnlock(&k->lock);
    return 0;
}

static inline void musashi_sdl_keyboard_open(MusashiSdlKeyboard *k,
                                             const uint16_t bindings[SDL_NUM_SCANCODES]) {
    SDL_Window *window = SDL_GL_GetCurrentWindow();
    if (!window) window = SDL_GetKeyboardFocus();
    memset(k, 0, sizeof *k);
    if (window) {
        k->window_id = SDL_GetWindowID(window);
        k->unfocused = !(SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS);
    }
    int count = 0;
    const Uint8 *keys = SDL_GetKeyboardState(&count);
    if (!k->unfocused && keys && count > 0) {
        if (count > SDL_NUM_SCANCODES) count = SDL_NUM_SCANCODES;
        memcpy(k->down, keys, (size_t)count);
    }
    memcpy(k->binding, bindings, sizeof k->binding);
    k->held = k->delivered = musashi_sdl_keyboard_held(k);
    SDL_AddEventWatch(musashi_sdl_keyboard_watch, k);
    k->watching = 1;
}

static inline void musashi_sdl_keyboard_close(MusashiSdlKeyboard *k) {
    if (k->watching) SDL_DelEventWatch(musashi_sdl_keyboard_watch, k);
    memset(k, 0, sizeof *k);
}

static inline void musashi_sdl_keyboard_set_bindings(MusashiSdlKeyboard *k,
                                                     const uint16_t bindings[SDL_NUM_SCANCODES]) {
    SDL_AtomicLock(&k->lock);
    memcpy(k->binding, bindings, sizeof k->binding);
    musashi_sdl_keyboard_flush(k);
    if (!k->suppressed && !k->unfocused)
        k->held = k->delivered = musashi_sdl_keyboard_held(k);
    SDL_AtomicUnlock(&k->lock);
}

static inline void musashi_sdl_keyboard_suppress(MusashiSdlKeyboard *k, int suppress) {
    SDL_AtomicLock(&k->lock);
    if (suppress && !k->suppressed) musashi_sdl_keyboard_flush(k);
    /* Only keys held before the modal and never released during it resume. */
    if (!suppress && k->suppressed && !k->unfocused)
        k->held = k->delivered = musashi_sdl_keyboard_held(k);
    k->suppressed = suppress != 0;
    SDL_AtomicUnlock(&k->lock);
}

/* A serial transaction previews one edge per button. A VBlank/UI peek cannot
 * replace that preview, and events arriving mid-report wait for a later poll. */
static inline uint16_t musashi_sdl_keyboard_report(MusashiSdlKeyboard *k, int sample) {
    uint16_t edges = 0;
    SDL_AtomicLock(&k->lock);
    for (unsigned bit = 0; bit < 16; ++bit)
        if (k->pending[bit]) edges |= (uint16_t)(1u << bit);
    if (sample) k->preview = edges;
    uint16_t buttons = k->delivered ^ edges;
    SDL_AtomicUnlock(&k->lock);
    return buttons;
}

static inline uint16_t musashi_sdl_keyboard_peek(MusashiSdlKeyboard *k) {
    return musashi_sdl_keyboard_report(k, 0);
}

static inline uint16_t musashi_sdl_keyboard_sample(MusashiSdlKeyboard *k) {
    return musashi_sdl_keyboard_report(k, 1);
}

static inline void musashi_sdl_keyboard_complete(MusashiSdlKeyboard *k) {
    SDL_AtomicLock(&k->lock);
    for (unsigned bit = 0; bit < 16; ++bit)
        if (k->preview & (1u << bit)) --k->pending[bit];
    k->delivered ^= k->preview;
    k->preview = 0;
    SDL_AtomicUnlock(&k->lock);
}

#endif
