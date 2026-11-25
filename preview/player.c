/* SPDX-License-Identifier: MIT */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "scene_views.h"
#include "desktop_voice.h"
#include "bfm_plat_font.h"
#include "title_flow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { TITLE_W = 640, TITLE_H = 480, OPENING_W = 320, OPENING_H = 240 };
/* On the US disc, the first visible loading sprite spans at least 115 fields. */
enum { LOADING_VISIBLE_FIELDS = 115 };
enum { PAD_START = 1u << 3, PAD_UP = 1u << 4, PAD_DOWN = 1u << 6,
       PAD_CIRCLE = 1u << 13, PAD_CROSS = 1u << 14 };

typedef struct Player {
    MusashiDiscMedia *disc;
    void *title, *opening;
    DesktopVoice *voice;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_GameController *controller;
    SDL_JoystickID controller_id;
    unsigned char keys[SDL_NUM_SCANCODES];
    uint16_t digital, held, pressed;
    int axis_y;
    uint64_t phase_start;
    BfmTitleFlow flow;
    int width, height;
    uint32_t pixels[TITLE_W * TITLE_H];
    int opening_finished, opening_seen, opening_skip;
    uint64_t voice_stop_field;
    int redraw;
} Player;

static const char usage[] =
    "Brave Fencer Musashi - native preview\n"
    "Usage: musashi_native_boot --disc path/to/game.cue [--windowed]\n"
    "Use your own US PlayStation disc image (SLUS-00726). A complete raw BIN or "
    "a CUE with all its BIN tracks alongside is supported.\n"
    "Enter/Start: start. Space/Cross: confirm or skip the opening. Up/Down: select. Escape: quit.\n";

static uint16_t button_mask(unsigned int button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_START: return PAD_START;
    case SDL_CONTROLLER_BUTTON_A: return PAD_CROSS;
    case SDL_CONTROLLER_BUTTON_B: return PAD_CIRCLE;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return PAD_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return PAD_DOWN;
    default: return 0;
    }
}

static void update_input(Player *p) {
    uint16_t current = p->digital;
    if (p->keys[SDL_SCANCODE_RETURN] || p->keys[SDL_SCANCODE_KP_ENTER]) current |= PAD_START;
    if (p->keys[SDL_SCANCODE_SPACE] || p->keys[SDL_SCANCODE_X]) current |= PAD_CROSS;
    if (p->keys[SDL_SCANCODE_Z] || p->keys[SDL_SCANCODE_BACKSPACE]) current |= PAD_CIRCLE;
    if (p->keys[SDL_SCANCODE_UP] || p->axis_y < -16384) current |= PAD_UP;
    if (p->keys[SDL_SCANCODE_DOWN] || p->axis_y > 16384) current |= PAD_DOWN;
    p->pressed |= current & (uint16_t)~p->held;
    p->held = current;
}

static void find_controller(Player *p) {
    int n = SDL_NumJoysticks(), i;
    for (i = 0; i < n && !p->controller; ++i) {
        if (SDL_IsGameController(i)) p->controller = SDL_GameControllerOpen(i);
    }
    if (p->controller) {
        p->controller_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p->controller));
        for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            if (SDL_GameControllerGetButton(p->controller, (SDL_GameControllerButton)i))
                p->digital |= button_mask((unsigned int)i);
        }
        p->axis_y = SDL_GameControllerGetAxis(p->controller, SDL_CONTROLLER_AXIS_LEFTY);
        update_input(p);
    }
}

static void phase_changed(Player *p) {
    const char *caption;
    switch (p->flow.phase) {
    case BFM_TITLE_PROMPT: caption = "Brave Fencer Musashi"; break;
    case BFM_TITLE_MENU: caption = "Brave Fencer Musashi - Start / Continue"; break;
    case BFM_TITLE_LOADING: caption = "Brave Fencer Musashi - Loading New Game"; break;
    case BFM_TITLE_OPENING: caption = "Brave Fencer Musashi - Opening"; break;
    case BFM_TITLE_CONTINUE_UNAVAILABLE:
        caption = "Brave Fencer Musashi - Continue unavailable in this preview"; break;
    default: return;
    }
    p->phase_start = SDL_GetTicks64();
    SDL_SetWindowTitle(p->window, caption);
    fprintf(stderr, "%s\n", caption);
}

static int events(Player *p) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: return 0;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (e.key.repeat) break;
            if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) return 0;
            if ((unsigned)e.key.keysym.scancode < SDL_NUM_SCANCODES)
                p->keys[e.key.keysym.scancode] = e.type == SDL_KEYDOWN;
            break;
        case SDL_AUDIODEVICEADDED:
        case SDL_AUDIODEVICEREMOVED:
            bfm_desktop_voice_event(p->voice, &e.adevice);
            break;
        case SDL_RENDER_DEVICE_RESET:
            if (p->texture) SDL_DestroyTexture(p->texture);
            p->texture = NULL;
            p->redraw = 1;
            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_EXPOSED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                p->redraw = 1;
            if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                memset(p->keys, 0, sizeof p->keys);
                p->pressed = 0;
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if (!p->controller) find_controller(p);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (p->controller && e.cdevice.which == p->controller_id) {
                SDL_GameControllerClose(p->controller);
                p->controller = NULL;
                p->controller_id = -1;
                p->digital = 0;
                p->axis_y = 0;
                p->pressed = 0;
                find_controller(p);
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (p->controller && e.caxis.which == p->controller_id &&
                e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) p->axis_y = e.caxis.value;
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            if (p->controller && e.cbutton.which == p->controller_id) {
                uint16_t mask = button_mask(e.cbutton.button);
                if (e.type == SDL_CONTROLLERBUTTONDOWN) p->digital |= mask;
                else p->digital &= (uint16_t)~mask;
            }
            break;
        }
        update_input(p);
    }
    return 1;
}

static int surface(Player *p, int width, int height) {
    if (p->texture && p->width == width && p->height == height) return 1;
    SDL_Texture *next = SDL_CreateTexture(p->renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!next) return 0;
    if (p->texture) SDL_DestroyTexture(p->texture);
    p->texture = next;
    p->width = width;
    p->height = height;
    return 1;
}

/* Status belongs to the preview. Add it only after the game scene stops or
 * when the player explicitly selects the unavailable Continue operation. */
static void status_line(Player *p, const char *text) {
    int scale = p->width / OPENING_W;
    int left = (p->width - (int)strlen(text) * BFM_FONT_CELL_W * scale) / 2;
    int top = p->height - 14 * scale;
    if (left < 0) left = 0;
    for (int y = top - 2 * scale; y < top + BFM_FONT_CELL_H * scale; ++y) {
        for (int x = 0; x < p->width; ++x) p->pixels[y * p->width + x] = 0xff101010u;
    }
    for (; *text && left + BFM_FONT_GLYPH_W * scale <= p->width; ++text, left += BFM_FONT_CELL_W * scale) {
        const uint8_t *glyph = bfm_plat_font_glyph(*text);
        for (int y = 0; y < BFM_FONT_GLYPH_H * scale; ++y) {
            for (int x = 0; x < BFM_FONT_GLYPH_W * scale; ++x) {
                if (glyph[y/scale] & (16u >> (x/scale)))
                    p->pixels[(top+y) * p->width + left+x] = 0xffffffffu;
            }
        }
    }
}

static int present(Player *p) {
    if (SDL_UpdateTexture(p->texture, NULL, p->pixels, p->width * (int)sizeof *p->pixels) < 0 ||
        SDL_RenderClear(p->renderer) < 0 || SDL_RenderCopy(p->renderer, p->texture, NULL, NULL) < 0)
        return 0;
    SDL_RenderPresent(p->renderer);
    return 1;
}

static int play(Player *p, char *error, size_t size) {
    uint64_t last_logic = SDL_GetTicks64() * 30u / 1001u;
    int last_phase = -1, last_choice = -1, last_prompt = -1;
    unsigned last_field = 0;
    bfm_title_begin(&p->flow, p->held);
    p->pressed = 0;
    phase_changed(p);
    while (events(p)) {
        uint64_t now = SDL_GetTicks64();
        uint64_t tick = now * 30u / 1001u;
        unsigned fields = (unsigned)((now - p->phase_start) * 60u / 1001u);
        int frame;
        if (tick != last_logic) {
            int before = p->flow.phase;
            if (p->flow.phase == BFM_TITLE_OPENING && !p->opening_skip &&
                (p->pressed & PAD_CROSS)) {
                int accepted = bfm_opening_request_skip(p->opening, p->disc, fields, error, size);
                if (accepted < 0) return 2;
                if (accepted) {
                    p->opening_skip = 1;
                    /* The outgoing voice becomes idle five fields after Cross. */
                    p->voice_stop_field = (uint64_t)fields + 5;
                    p->opening_finished = 0;
                    SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Chapter 1");
                }
            } else if (p->opening_finished && (p->pressed & (PAD_START | PAD_CROSS | PAD_CIRCLE))) {
                bfm_desktop_voice_stop(p->voice);
                bfm_opening_free(p->opening);
                p->opening = NULL;
                p->opening_finished = 0;
                p->opening_seen = 0;
                p->opening_skip = 0;
                p->voice_stop_field = 0;
                bfm_title_begin(&p->flow, p->held | p->pressed);
            } else {
                bfm_title_input(&p->flow, p->held | p->pressed);
            }
            p->pressed = 0;
            if ((int)p->flow.phase != before) phase_changed(p);
            last_logic = tick;
        }
        fields = (unsigned)((SDL_GetTicks64() - p->phase_start) * 60u / 1001u);
        if (p->flow.phase == BFM_TITLE_LOADING && p->opening && fields >= LOADING_VISIBLE_FIELDS) {
            bfm_title_finish_loading(&p->flow, 1);
            phase_changed(p);
            fields = 0;
        }
        int opening = p->flow.phase == BFM_TITLE_OPENING;
        if (!surface(p, opening ? OPENING_W : TITLE_W, opening ? OPENING_H : TITLE_H)) {
            snprintf(error, size, "Could not create the scene display: %s", SDL_GetError());
            return 2;
        }
        int unchanged = last_phase == (int)p->flow.phase && last_choice == (int)p->flow.choice &&
                        last_prompt == p->flow.prompt_visible && (!opening || fields == last_field);
        if (unchanged || p->opening_finished) {
            if (p->redraw && !present(p)) {
                snprintf(error, size, "Could not refresh the display: %s", SDL_GetError());
                return 2;
            }
            p->redraw = 0;
            SDL_Delay(8);
            continue;
        }
        last_phase = p->flow.phase;
        last_choice = p->flow.choice;
        last_prompt = p->flow.prompt_visible;
        last_field = fields;
        p->redraw = 0;
        if (opening) {
            unsigned bank, cue;
            if (p->voice_stop_field && fields >= p->voice_stop_field) {
                bfm_desktop_voice_stop(p->voice);
                p->voice_stop_field = 0;
            }
            if (bfm_opening_voice_due(p->opening, fields, &bank, &cue) &&
                bfm_desktop_voice_request(p->voice, bank, cue))
                fprintf(stderr, "Palace voice %u/%u requested at field %u\n", bank, cue, fields);
            if (bfm_desktop_voice_pump(p->voice, &bank, &cue)) {
                bfm_opening_voice_started(p->opening, bank, cue, fields);
                fprintf(stderr, "Palace voice %u/%u reached SDL output at field %u\n", bank, cue, fields);
            }
            frame = bfm_opening_frame(p->opening, fields, p->pixels, p->width, p->height);
            if (frame == BFM_OPENING_FRAME) p->opening_seen = 1;
            if (frame == BFM_OPENING_END || frame == BFM_OPENING_FOREST_PENDING) {
                if (!p->opening_seen) {
                    snprintf(error, size, "The opening ended before it produced its first frame.");
                    return 2;
                }
                if (frame == BFM_OPENING_END &&
                    bfm_opening_final_frame(p->opening, p->pixels, p->width, p->height) <= 0) {
                    snprintf(error, size, "Could not render the final opening frame.");
                    return 2;
                }
                bfm_desktop_voice_stop(p->voice);
                p->opening_finished = 1;
                SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - End of opening preview");
                fputs("End of the implemented opening. Gameplay control and saving are unavailable.\n", stderr);
                if (frame == BFM_OPENING_FOREST_PENDING)
                    status_line(p, "Chapter 1 card complete. Start returns.");
                else
                    status_line(p, "End of preview. Press Start to return.");
                if (!present(p)) {
                    snprintf(error, size, "Could not display the ending: %s", SDL_GetError());
                    return 2;
                }
                continue;
            }
        } else {
            frame = bfm_title_frame(p->title, fields, p->flow.phase, p->flow.choice,
                                    p->flow.prompt_visible, p->pixels, p->width, p->height);
        }
        if (p->flow.phase == BFM_TITLE_LOADING && frame == 0) {
            memset(p->pixels, 0, (size_t)p->width * p->height * sizeof *p->pixels);
            status_line(p, "Loading New Game...");
            frame = 1;
        }
        if (p->flow.phase == BFM_TITLE_CONTINUE_UNAVAILABLE && frame >= 0) {
            status_line(p, "Continue unavailable. Press Start to return.");
            frame = 1;
        }
        if (frame <= 0) {
            snprintf(error, size, "Could not render the current scene from your disc.");
            return 2;
        }
        if (frame && !present(p)) {
            snprintf(error, size, "SDL display failed: %s", SDL_GetError());
            return 2;
        }
        if (p->flow.phase == BFM_TITLE_LOADING && !p->opening) {
            p->phase_start = SDL_GetTicks64();
            if (p->flow.room != 0x3005 || !bfm_opening_load(p->disc, &p->opening, error, size))
                return 2;
        }
        SDL_Delay(8);
    }
    return 0;
}

static int run(const char *disc_path, int windowed) {
    char error[512] = {0};
    int status = 2;
    Player *p = calloc(1, sizeof *p);
    int sdl_ready = 0;
    if (!p) { fputs("Not enough memory to start.\n", stderr); return 2; }
    p->disc = musashi_disc_media_open_image(disc_path, error, sizeof error);
    if (!p->disc) {
        fprintf(stderr, "could not start from your disc image: %s\n", error);
        free(p);
        return 2;
    }
    if (!bfm_title_load(p->disc, &p->title, error, sizeof error)) goto done;
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) goto sdl_error;
    sdl_ready = 1;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) p->voice = bfm_desktop_voice_open(p->disc);
    else SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Dialogue output unavailable: %s", SDL_GetError());
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    p->window = SDL_CreateWindow("Brave Fencer Musashi", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, TITLE_W, TITLE_H,
                                  SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE |
                                      (windowed ? 0u : SDL_WINDOW_FULLSCREEN_DESKTOP));
    if (!p->window) goto sdl_error;
    p->renderer = SDL_CreateRenderer(p->window, -1, SDL_RENDERER_ACCELERATED);
    if (!p->renderer) p->renderer = SDL_CreateRenderer(p->window, -1, SDL_RENDERER_SOFTWARE);
    if (!p->renderer || SDL_RenderSetLogicalSize(p->renderer, TITLE_W, TITLE_H) < 0) goto sdl_error;
    SDL_SetRenderDrawColor(p->renderer, 0, 0, 0, 255);
    SDL_ShowCursor(SDL_DISABLE);
    find_controller(p);
    status = play(p, error, sizeof error);
    goto done;
sdl_error:
    snprintf(error, sizeof error, "SDL could not start the display: %s", SDL_GetError());
done:
    if (status != 0) fprintf(stderr, "%s\n", *error ? error : "The game could not start a scene.");
    if (p->controller) SDL_GameControllerClose(p->controller);
    if (p->texture) SDL_DestroyTexture(p->texture);
    if (p->renderer) SDL_DestroyRenderer(p->renderer);
    if (p->window) SDL_DestroyWindow(p->window);
    bfm_desktop_voice_free(p->voice);
    if (sdl_ready) SDL_Quit();
    if (p->opening) bfm_opening_free(p->opening);
    if (p->title) bfm_title_free(p->title);
    musashi_disc_media_close(p->disc);
    free(p);
    return status;
}

int main(int argc, char **argv) {
    const char *disc = NULL;
    int i, windowed = 0;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            fputs(usage, stdout);
            return 0;
        }
        if (!strcmp(argv[i], "--windowed")) windowed = 1;
        else if (!strcmp(argv[i], "--disc") && i + 1 < argc && !disc) disc = argv[++i];
        else if (argv[i][0] != '-' && !disc) disc = argv[i];
        else { fputs(usage, stderr); return 2; }
    }
    if (!disc) disc = getenv("BFM_DISC");
    if (!disc || !*disc) { fputs(usage, stderr); return 2; }
    return run(disc, windowed);
}
