/* SPDX-License-Identifier: MIT */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "scene_views.h"
#include "desktop_voice.h"
#include "bfm_plat_font.h"
#include "title_flow.h"
#include "save_menu.h"
#include "chapter_completion_view.h"
#include "inn_profile.h"
#include "inn_menu.h"
#include "inn_field.h"
#include "forest_walk.h"
#include "gameplay_run.h"
#include "bedroom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { TITLE_W = 640, TITLE_H = 480, OPENING_W = 320, OPENING_H = 240 };
/* On the US disc, the first visible loading sprite spans at least 115 fields. */
enum { LOADING_VISIBLE_FIELDS = 115, CARD_READ_VISIBLE_FIELDS = 90 };
enum { PAD_START = 1u << 3, PAD_UP = 1u << 4, PAD_RIGHT = 1u << 5, PAD_DOWN = 1u << 6,
       PAD_LEFT = 1u << 7, PAD_TRIANGLE = 1u << 12,
       PAD_CIRCLE = BFM_FOREST_CIRCLE, PAD_CROSS = BFM_FOREST_CROSS, PAD_SQUARE = BFM_FOREST_SQUARE };

typedef struct Player {
    MusashiDiscMedia *disc;
    void *title, *opening;
    DesktopVoice *voice;
    BfmForestWalk *forest;
    BfmGameplayRun gameplay;
    uint64_t forest_start;
    unsigned forest_updates;
    uint16_t forest_pressed;
    int control_message;
    uint16_t display_room;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_GameController *controller;
    SDL_JoystickID controller_id;
    unsigned char keys[SDL_NUM_SCANCODES];
    uint16_t digital, held, pressed;
    int axis_x, axis_y;
    const char *card_path;
    BfmInnProfile *save_copy;
    BfmInnMenu inn_menu;
    unsigned inn_age;
    int inn_service;
    BfmSaveMenu saves;
    BfmSaveArt *save_art;
    BfmChapterCompletion chapter;
    BfmChapterCard *chapter_title;
    unsigned chapter_fields;
    BfmBedroom *bedroom;
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
    "Usage: musashi_native_boot --disc path/to/game.cue [--card path/to/card.mcd [--save-copy new-directory]] [--windowed]\n"
    "Use your own US PlayStation disc image (SLUS-00726). A complete raw BIN or "
    "a CUE with all its BIN tracks alongside is supported.\n"
    "Continue reads a supplied raw 128 KiB card and supports the early castle bedroom wake.\n"
    "--save-copy starts a writable copy in a new directory. The --card input remains read-only.\n"
    "Enter/Start: start. Space/Cross: confirm or skip the opening. Arrows: select. "
    "D-pad, arrows or left stick: move in Giant Forest. A/Square: swing Fusion on the ground. "
    "Right Shift/R1: charge Fusion. Z/Circle: fire earned Gunshot. T/Triangle: back from files. Escape: quit.\n";

static uint16_t button_mask(unsigned int button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_START: return PAD_START;
    case SDL_CONTROLLER_BUTTON_A: return PAD_CROSS;
    case SDL_CONTROLLER_BUTTON_B: return PAD_CIRCLE;
    case SDL_CONTROLLER_BUTTON_X: return PAD_SQUARE;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return BFM_FOREST_R1;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return PAD_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return PAD_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return PAD_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return PAD_RIGHT;
    case SDL_CONTROLLER_BUTTON_Y: return PAD_TRIANGLE;
    default: return 0;
    }
}

/* Digital directions take priority on their axis. Opposing presses cancel. */
static int field_axis(int negative, int positive, int analog) {
    if (negative || positive) return 127 * (!!positive - !!negative);
    return analog * 127 / (analog < 0 ? 32768 : 32767);
}

static void update_input(Player *p) {
    uint16_t current = p->digital;
    if (p->keys[SDL_SCANCODE_RSHIFT]) current |= BFM_FOREST_R1;
    if (p->keys[SDL_SCANCODE_RETURN] || p->keys[SDL_SCANCODE_KP_ENTER]) current |= PAD_START;
    if (p->keys[SDL_SCANCODE_SPACE] || p->keys[SDL_SCANCODE_X]) current |= PAD_CROSS;
    if (p->keys[SDL_SCANCODE_Z] || p->keys[SDL_SCANCODE_BACKSPACE]) current |= PAD_CIRCLE;
    if (p->keys[SDL_SCANCODE_UP] || p->axis_y < -16384) current |= PAD_UP;
    if (p->keys[SDL_SCANCODE_DOWN] || p->axis_y > 16384) current |= PAD_DOWN;
    if (p->keys[SDL_SCANCODE_LEFT] || p->axis_x < -16384) current |= PAD_LEFT;
    if (p->keys[SDL_SCANCODE_RIGHT] || p->axis_x > 16384) current |= PAD_RIGHT;
    if (p->keys[SDL_SCANCODE_T]) current |= PAD_TRIANGLE;
    if (p->keys[SDL_SCANCODE_A]) current |= PAD_SQUARE;
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
        p->axis_x = SDL_GameControllerGetAxis(p->controller, SDL_CONTROLLER_AXIS_LEFTX);
        p->axis_y = SDL_GameControllerGetAxis(p->controller, SDL_CONTROLLER_AXIS_LEFTY);
        update_input(p);
    }
}

static void phase_changed(Player *p) {
    const char *caption;
    switch (p->flow.phase) {
    case BFM_TITLE_PROMPT: caption = "Brave Fencer Musashi"; break;
    case BFM_TITLE_MENU: caption = "Brave Fencer Musashi - Start / Continue"; break;
    case BFM_TITLE_LOADING:
        p->opening_finished = p->opening_seen = p->opening_skip = 0;
        p->voice_stop_field = 0;
        p->gameplay.has_player = 0;
        memset(p->gameplay.story_flags, 0, sizeof p->gameplay.story_flags);
        caption = "Brave Fencer Musashi - Loading New Game";
        break;
    case BFM_TITLE_OPENING: caption = "Brave Fencer Musashi - Opening"; break;
    case BFM_TITLE_CHAPTER_COMPLETION: caption = "Brave Fencer Musashi - Chapter complete"; break;
    case BFM_TITLE_BEDROOM: caption = "Brave Fencer Musashi - Castle bedroom"; break;
    case BFM_TITLE_FILES:
        if (p->save_copy) bfm_inn_profile_menu(p->save_copy, &p->saves);
        else bfm_save_menu_open(&p->saves, p->card_path);
        caption = "Brave Fencer Musashi - Saved files"; break;
    default: return;
    }
    p->phase_start = SDL_GetTicks64();
    if (p->flow.phase == BFM_TITLE_OPENING)
        bfm_desktop_voice_opening(p->voice, p->phase_start);
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
                p->axis_x = p->axis_y = 0;
                p->pressed = 0;
                find_controller(p);
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (p->controller && e.caxis.which == p->controller_id) {
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) p->axis_y = e.caxis.value;
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) p->axis_x = e.caxis.value;
            }
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

/* Draw the desktop status when mounting a scene or reaching the opening endpoint. */
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

static int bedroom_frame(Player *p) {
    uint32_t (*pixels)[320] = (uint32_t (*)[320])p->pixels;
    if (p->inn_service) {
        int at_counter = bfm_inn_menu_dialogue(&p->inn_menu);
        if (!bfm_bedroom_draw_counter(p->bedroom, at_counter, pixels)) return -1;
        bfm_inn_menu_draw(&p->inn_menu, p->save_art, p->inn_age * 2u, pixels);
    } else if (!bfm_bedroom_draw(p->bedroom, pixels)) return -1;
    if (bfm_bedroom_endpoint(p->bedroom))
        status_line(p, bfm_bedroom_end_message(p->bedroom));
    return 1;
}

static int keeper_service(Player *p, unsigned pressed, int previously_open)
{
    const BfmSaveRecord *record = bfm_bedroom_inn_record(p->bedroom);
    if (!record) {
        p->inn_service = 0;
        return 1;
    }
    if (!previously_open) {
        if (!p->save_art) p->save_art = bfm_save_art_load(p->disc);
        if (!p->save_art) return 0;
        if (p->save_copy) bfm_inn_profile_menu(p->save_copy, &p->saves);
        bfm_inn_menu_begin(&p->inn_menu, p->save_copy ? &p->saves : NULL);
        BfmInnField *inn = bfm_bedroom_inn_counter(p->bedroom);
        for (unsigned i = 0; i < 6; ++i)
            p->inn_menu.prices[i] = bfm_inn_room_price(inn, i);
        p->inn_service = 1;
        p->inn_age = 0;
        return 1;
    }
    ++p->inn_age;
    if (p->inn_menu.page == BFM_INN_WRITING) {
        unsigned file = p->inn_menu.card.selected + 1;
        const char *error = bfm_inn_profile_save(p->save_copy, file, record, p->disc);
        if (!error) bfm_inn_profile_menu(p->save_copy, &p->saves);
        bfm_inn_menu_complete(&p->inn_menu, error ? NULL : &p->saves, error);
        return 1;
    }
    BfmInnAction action = bfm_inn_menu_input(&p->inn_menu, pressed);
    if (action == BFM_INN_STAY) {
        bfm_inn_menu_stay(&p->inn_menu, bfm_inn_pay_room201(bfm_bedroom_inn_counter(p->bedroom)));
    } else if (action == BFM_INN_RECOVER) {
        bfm_inn_finish_room201(bfm_bedroom_inn_counter(p->bedroom));
    } else if (action == BFM_INN_LEAVE) {
        p->inn_service = 0;
        bfm_desktop_voice_room_skip(p->voice);
        return bfm_bedroom_leave_inn(p->bedroom);
    }
    return 1;
}

static int present_catchup_room(Player *p, uint64_t batch_start, uint64_t updates,
                                uint64_t *next_present) {
    uint64_t start = SDL_GetTicks64();
    if (start < *next_present) return 1;
    if (!surface(p, OPENING_W, OPENING_H) || bedroom_frame(p) < 0 || !present(p)) return 0;

    /* While paying off missed 30 Hz updates, submit each picture if the
     * renderer can keep twice that pace. Otherwise allow a room tick, and
     * at least the cost of the last submission, for more game updates. */
    uint64_t end = SDL_GetTicks64();
    if ((end - batch_start) * 60u > updates * 1001u) {
        uint64_t cost = end - start;
        *next_present = end + SDL_max(cost, (1001u + 29u) / 30u);
    }
    return 1;
}

/* Only the ordinary opening event supplies the source member. Its two
 * supported rooms queue distinct authored transfers to the same forest. */
static int start_forest(Player *p, unsigned field)
{
    uint64_t forest_field;
    p->forest = bfm_opening_take_forest(p->opening, p->disc, field, &forest_field);
    if (!p->forest) return 0;
    if (!bfm_forest_walk_bind_gameplay(p->forest, &p->gameplay)) return 0;
    bfm_opening_free(p->opening);
    p->opening = NULL;
    p->forest_start = forest_field ? p->phase_start + (forest_field * 1001 + 59) / 60
                                   : SDL_GetTicks64();
    p->forest_updates = 0;
    p->forest_pressed = 0;
    p->control_message = 0;
    p->display_room = bfm_forest_walk_room(p->forest);
    SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Giant Forest");
    return 1;
}

/* The forest event is consumed only after its final field has run. Stop the
 * outgoing audio before releasing its scene-owned voices and instruments. */
static int begin_completed_chapter(Player *p)
{
    if (!p->save_art) p->save_art = bfm_save_art_load(p->disc);
    if (!p->save_art) return -1;
    p->chapter_title = bfm_chapter_two_card_load(p->disc);
    if (!p->chapter_title || !bfm_chapter_completion_begin_earned(&p->chapter, &p->gameplay))
        return -1;

    bfm_desktop_voice_stop(p->voice);
    bfm_forest_walk_free(p->forest);
    p->forest = NULL;
    p->pressed = p->forest_pressed = 0;
    p->chapter_fields = 0;
    p->flow.phase = BFM_TITLE_CHAPTER_COMPLETION;
    phase_changed(p);
    return bfm_chapter_completion_draw(&p->chapter, p->save_art, p->chapter_title, 0,
                                       (uint32_t (*)[OPENING_W])p->pixels) ? BFM_OPENING_FRAME : -1;
}

static int advance_completed_chapter(Player *p, unsigned fields)
{
    while (p->chapter_fields < fields) {
        ++p->chapter_fields;
        uint16_t pressed = p->pressed;
        p->pressed = 0;
        BfmChapterSavePage before = p->chapter.page;
        /* An unsupported Back choice leaves its prompt on screen. */
        if (!bfm_chapter_completion_step(&p->chapter, p->disc, p->save_copy, pressed) &&
            !(pressed & PAD_TRIANGLE)) return 0;
        if (p->chapter.page == BFM_CHAPTER_TWO_TITLE && before != BFM_CHAPTER_TWO_TITLE)
            SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Chapter 2");
        if (p->chapter.page == BFM_CHAPTER_BEDROOM) {
            p->bedroom = bfm_chapter_completion_take_bedroom(&p->chapter, p->disc);
            if (!p->bedroom || !bfm_bedroom_step(p->bedroom)) return 0;
            bfm_chapter_card_free(p->chapter_title);
            p->chapter_title = NULL;
            p->chapter = (BfmChapterCompletion){0};
            p->flow.phase = BFM_TITLE_BEDROOM;
            phase_changed(p);
            break;
        }
    }
    return 1;
}

static int forest_frame(Player *p)
{
    uint64_t wanted = (SDL_GetTicks64() - p->forest_start) * 30u / 1001u;
    int running = 1;
    while (p->forest_updates < wanted) {
        int x = !!(p->held & PAD_RIGHT) - !!(p->held & PAD_LEFT);
        int y = !!(p->held & PAD_DOWN) - !!(p->held & PAD_UP);
        int stick = p->axis_x < -16384 || p->axis_x > 16384 ||
                    p->axis_y < -16384 || p->axis_y > 16384;
        running = bfm_forest_walk_tick_controller(p->forest, x, y, p->forest_pressed, p->held, stick);
        p->forest_pressed = 0;
        if (running < 0) return -1;
        if (bfm_forest_walk_swing_sound(p->forest)) bfm_desktop_voice_forest_swing(p->voice);
        unsigned bell_command, bell_argument;
        struct BfmTowerBellSound *bell = bfm_forest_walk_bell_sound(p->forest, &bell_command, &bell_argument);
        if (bell) bfm_desktop_voice_tower_bell(p->voice, bell, bell_command, bell_argument);
        if (bfm_forest_walk_gunshot_sound(p->forest)) bfm_desktop_voice_forest_gunshot(p->voice);
        if (bfm_forest_walk_hit_sound(p->forest)) bfm_desktop_voice_forest_hit(p->voice);
        if (bfm_forest_walk_take_chapter_request(p->forest))
            return begin_completed_chapter(p);
        if (!running) break;
        ++p->forest_updates;
    }
    int control = bfm_forest_walk_control(p->forest);
    uint16_t room = bfm_forest_walk_room(p->forest);
    if (control != p->control_message || room != p->display_room) {
        if (room == 0x300e) {
            SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Allucaneet Castle");
        } else if (room == 0x300d) {
            SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Allucaneet Castle drawbridge");
        } else if (room == 0x300b) {
            const char *title = control == 0 ? "Brave Fencer Musashi - Arriving at the Spiral Tower summit" :
                                control == 2 ? "Brave Fencer Musashi - Spiral Tower summit preview boundary" :
                                               "Brave Fencer Musashi - Spiral Tower summit";
            SDL_SetWindowTitle(p->window, title);
        } else if (room == 0x300a) {
            const char *title = control == 0 ? "Brave Fencer Musashi - Entering Spiral Tower interior" :
                                control == 2 ? "Brave Fencer Musashi - Spiral Tower interior preview boundary" :
                                               "Brave Fencer Musashi - Spiral Tower interior";
            SDL_SetWindowTitle(p->window, title);
        } else if (room == 0x3009) {
            const char *title = control == 0 ? "Brave Fencer Musashi - Entering Spiral Tower" :
                                control == 2 ? "Brave Fencer Musashi - Spiral Tower preview boundary" :
                                               "Brave Fencer Musashi - Spiral Tower";
            SDL_SetWindowTitle(p->window, title);
        } else if (control == 2)
            SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Forest preview boundary");
        else if (control == 1)
            SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Giant Forest movement");
        p->control_message = control;
        p->display_room = room;
    }
    unsigned generation = 0;
    struct ForestVoice *source = bfm_forest_walk_voice(p->forest, &generation);
    bfm_desktop_voice_forest(p->voice, source, generation, 2 * p->forest_updates);
    if (!bfm_forest_walk_draw(p->forest, (uint32_t (*)[OPENING_W])p->pixels)) return -1;
    return running ? BFM_OPENING_FRAME : BFM_OPENING_END;
}

static int play(Player *p, char *error, size_t size) {
    uint64_t last_logic = SDL_GetTicks64() * 30u / 1001u;
    int last_phase = -1, last_choice = -1, last_prompt = -1, last_save_view = -1;
    unsigned last_field = 0;
    bfm_title_begin(&p->flow, p->held);
    p->pressed = 0;
    phase_changed(p);
    while (events(p)) {
        uint64_t now = SDL_GetTicks64();
        uint64_t tick = now * 30u / 1001u;
        unsigned fields = (unsigned)((now - p->phase_start) * 60u / 1001u);
        int frame, room_advanced = 0;
        if (p->flow.phase == BFM_TITLE_OPENING && p->opening && !p->opening_finished) {
            int began = bfm_opening_advance_court(p->opening, p->disc, fields, error, size);
            if (began < 0) return 2;
            if (began) SDL_SetWindowTitle(p->window, "Brave Fencer Musashi - Chapter 1");
        }
        if (p->flow.phase == BFM_TITLE_CHAPTER_COMPLETION) {
            if (!advance_completed_chapter(p, fields)) {
                snprintf(error, size, "Could not advance the chapter-completion scene.");
                return 2;
            }
            if (p->flow.phase == BFM_TITLE_BEDROOM) fields = 0;
            last_logic = tick;
        } else if (tick != last_logic) {
            int before = p->flow.phase;
            if (p->flow.phase == BFM_TITLE_OPENING && p->opening && !p->opening_skip &&
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
                bfm_forest_walk_free(p->forest);
                p->forest = NULL;
                bfm_opening_free(p->opening);
                p->opening = NULL;
                p->opening_finished = 0;
                p->opening_seen = 0;
                p->opening_skip = 0;
                p->voice_stop_field = 0;
                bfm_title_begin(&p->flow, p->held | p->pressed);
            } else if (p->flow.phase == BFM_TITLE_FILES) {
                BfmSavePage previous = p->saves.page;
                if (previous == BFM_SAVE_LOADED && (p->pressed & PAD_CROSS)) {
                    p->flow.phase = BFM_TITLE_BEDROOM;
                    if (!bfm_bedroom_step(p->bedroom)) {
                        snprintf(error, size, "Could not start the mounted bedroom scene.");
                        return 2;
                    }
                } else if (bfm_save_menu_input(&p->saves, p->pressed)) {
                    if (p->saves.issue) {
                        p->flow.phase = BFM_TITLE_MENU;
                        p->flow.idle_ticks = 0;
                    } else {
                        bfm_title_begin(&p->flow, p->held | p->pressed);
                    }
                }
                if (previous == BFM_SAVE_CONFIRM && p->saves.page == BFM_SAVE_BOUNDARY) {
                    p->bedroom = bfm_bedroom_open(p->disc, &p->saves.records[p->saves.selected]);
                    if (p->bedroom) {
                        bfm_bedroom_bind_run(p->bedroom, &p->gameplay);
                        p->saves.page = BFM_SAVE_LOADED;
                    }
                } else if (previous == BFM_SAVE_LOADED && p->saves.page == BFM_SAVE_PICK) {
                    bfm_bedroom_free(p->bedroom);
                    p->bedroom = NULL;
                }
                p->flow.held = p->held | p->pressed;
            } else if (p->forest) {
                p->forest_pressed |= p->pressed;
            } else if (p->flow.phase == BFM_TITLE_BEDROOM) {
                if (bfm_bedroom_endpoint(p->bedroom) && (p->pressed & (PAD_TRIANGLE | PAD_START | PAD_CROSS))) {
                    bfm_desktop_voice_stop(p->voice);
                    bfm_bedroom_free(p->bedroom);
                    p->bedroom = NULL;
                    bfm_title_begin(&p->flow, p->held | p->pressed);
                } else {
                    uint64_t next_present = 0;
                    for (uint64_t t = last_logic; t < tick; ++t) {
                        if (t != last_logic &&
                            !present_catchup_room(p, now, t - last_logic, &next_present)) {
                            snprintf(error, size, "Could not present the mounted room during catch-up.");
                            return 2;
                        }
                        unsigned pressed = t == last_logic ? p->pressed : 0;
                        int service = p->inn_service;
                        unsigned scene_input = service ? 0 : pressed;
                        int direction = (scene_input & PAD_UP) ? -1 : !!(scene_input & PAD_DOWN);
                        int held = (p->held & PAD_UP) ? -1 : !!(p->held & PAD_DOWN);
                        int x = field_axis(p->keys[SDL_SCANCODE_LEFT] || (p->digital & PAD_LEFT),
                                           p->keys[SDL_SCANCODE_RIGHT] || (p->digital & PAD_RIGHT), p->axis_x);
                        int y = field_axis(p->keys[SDL_SCANCODE_UP] || (p->digital & PAD_UP),
                                           p->keys[SDL_SCANCODE_DOWN] || (p->digital & PAD_DOWN), p->axis_y);
                        int result = bfm_bedroom_step_axes(p->bedroom,
                                    scene_input & (PAD_CIRCLE | PAD_CROSS | PAD_TRIANGLE),
                                    scene_input & PAD_CIRCLE, direction, held, scene_input & PAD_CROSS,
                                    scene_input & PAD_TRIANGLE, x, y, scene_input & PAD_SQUARE);
                        if (!result || !keeper_service(p, pressed, service)) {
                            snprintf(error, size, "Could not advance the mounted room.");
                            return 2;
                        }
                        if (result == BFM_BEDROOM_VOICE_STOPPED) bfm_desktop_voice_room_skip(p->voice);
                        bfm_desktop_voice_room_score(p->voice, bfm_bedroom_score(p->bedroom));
                        int slider_event;
                        struct BfmCableSound *slider=bfm_bedroom_slider(p->bedroom,&slider_event);
                        bfm_desktop_voice_slider(p->voice,slider,slider_event);
                        bfm_desktop_voice_landing(p->voice, bfm_bedroom_landing(p->bedroom));
                        unsigned command;
                        struct BfmBedroomEffects *effects = bfm_bedroom_effect(p->bedroom, &command);
                        bfm_desktop_voice_room_effect(p->voice, effects, command);
                        unsigned generation;
                        struct ForestVoice *voice = bfm_bedroom_voice(p->bedroom, &generation);
                        bfm_desktop_voice_forest(p->voice, voice, generation,
                                                 bfm_bedroom_voice_ready_field(p->bedroom));
                    }
                    room_advanced = 1;
                }
            } else {
                bfm_title_input(&p->flow, p->held | p->pressed);
            }
            p->pressed = 0;
            if ((int)p->flow.phase != before) {
                phase_changed(p);
                fields = 0;
            }
            last_logic = tick;
        }
        if (p->flow.phase == BFM_TITLE_LOADING && p->opening && fields >= LOADING_VISIBLE_FIELDS) {
            bfm_title_finish_loading(&p->flow, 1);
            phase_changed(p);
            fields = 0;
        }
        int opening = p->flow.phase == BFM_TITLE_OPENING;
        int chapter = p->flow.phase == BFM_TITLE_CHAPTER_COMPLETION;
        int files = p->flow.phase == BFM_TITLE_FILES;
        int bedroom = p->flow.phase == BFM_TITLE_BEDROOM;
        if (files && p->saves.page == BFM_SAVE_CARD_LOADING && fields >= LOADING_VISIBLE_FIELDS)
            p->saves.page = BFM_SAVE_CARD_READING;
        if (files && p->saves.page == BFM_SAVE_CARD_READING &&
            fields >= LOADING_VISIBLE_FIELDS + CARD_READ_VISIBLE_FIELDS)
            p->saves.page = BFM_SAVE_NO_DATA;
        if (files && !p->save_art) {
            p->save_art = bfm_save_art_load(p->disc);
            if (!p->save_art) {
                snprintf(error, size, "Could not read file-selection artwork from your disc.");
                return 2;
            }
        }
        int save_view = files ? (int)p->saves.selected + 4 * (int)p->saves.page : -1;
        int field_view = opening || files || bedroom || chapter;
        if (!surface(p, field_view ? OPENING_W : TITLE_W, field_view ? OPENING_H : TITLE_H)) {
            snprintf(error, size, "Could not create the scene display: %s", SDL_GetError());
            return 2;
        }
        int unchanged = last_phase == (int)p->flow.phase && last_choice == (int)p->flow.choice &&
                        last_prompt == p->flow.prompt_visible && last_save_view == save_view &&
                        (!files || fields / 2 == last_field / 2) && !room_advanced &&
                        (!(opening || chapter) || fields == last_field);
        if (unchanged || p->opening_finished) {
            if (p->redraw && !present(p)) {
                snprintf(error, size, "Could not refresh the display: %s", SDL_GetError());
                return 2;
            }
            p->redraw = 0;
            SDL_Delay(8);
            continue;
        }
        last_save_view = save_view;
        last_phase = p->flow.phase;
        last_choice = p->flow.choice;
        last_prompt = p->flow.prompt_visible;
        last_field = fields;
        p->redraw = 0;
        if (opening) {
            if (p->forest) {
                frame = forest_frame(p);
            } else {
                unsigned bank, cue, issue_field;
                if (p->voice_stop_field && fields >= p->voice_stop_field) {
                    bfm_desktop_voice_stop(p->voice);
                    p->voice_stop_field = 0;
                }
                if (bfm_opening_voice_schedule(p->opening, fields, &bank, &cue, &issue_field) &&
                    bfm_desktop_voice_request(p->voice, bank, cue, issue_field))
                    fprintf(stderr, "Palace voice %u/%u requested at field %u (handled at %u)\n",
                            bank, cue, issue_field, fields);
                int voice_events = bfm_desktop_voice_pump(p->voice, fields, &bank, &cue);
                if (voice_events & BFM_VOICE_MESSAGE_READY) {
                    unsigned ready = bfm_desktop_voice_ready_field(p->voice);
                    bfm_opening_voice_started(p->opening, bank, cue, ready);
                    fprintf(stderr, "Palace voice %u/%u ready for the message at field %u (handled at %u)\n",
                            bank, cue, ready, fields);
                }
                if (voice_events & BFM_VOICE_OUTPUT_STARTED) {
                    fprintf(stderr, "Palace voice %u/%u reached SDL output at field %u\n", bank, cue, fields);
                }
                uint64_t score_field;
                if (bfm_opening_forest_score_field(p->opening, fields, &score_field))
                    bfm_desktop_voice_forest_score(p->voice, p->disc,
                                                   p->phase_start + (score_field * 1001 + 59) / 60);
                frame = bfm_opening_frame(p->opening, fields, p->pixels, p->width, p->height);
                if (frame == BFM_OPENING_FOREST_PENDING) {
                    if (!start_forest(p, fields)) {
                        snprintf(error, size, "Could not load the selected forest entrance from your disc.");
                        return 2;
                    }
                    frame = forest_frame(p);
                }
            }
            if (frame == BFM_OPENING_FRAME) p->opening_seen = 1;
            if (frame == BFM_OPENING_END) {
                if (!p->opening_seen) {
                    snprintf(error, size, "The opening ended before it produced its first frame.");
                    return 2;
                }
                if (!p->forest &&
                    bfm_opening_final_frame(p->opening, p->pixels, p->width, p->height) <= 0) {
                    snprintf(error, size, "Could not render the final opening frame.");
                    return 2;
                }
                if (!p->forest) bfm_desktop_voice_finish(p->voice);
                p->opening_finished = 1;
                int castle_gate = p->forest && bfm_forest_walk_room(p->forest) == 0x300d;
                SDL_SetWindowTitle(p->window, castle_gate ? "Brave Fencer Musashi - Allucaneet Castle gate" :
                                              p->forest ? "Brave Fencer Musashi - Giant Forest preview" :
                                                          "Brave Fencer Musashi - End of opening preview");
                fputs("End of the implemented opening scene. Press Start to return.\n", stderr);
                if (castle_gate)
                    status_line(p, "Castle gate reached. Start returns.");
                else if (p->forest)
                    status_line(p, "Forest dialogue complete. Start returns.");
                else
                    status_line(p, "End of preview. Press Start to return.");
                if (!present(p)) {
                    snprintf(error, size, "Could not display the ending: %s", SDL_GetError());
                    return 2;
                }
                continue;
            }
        } else if (chapter) {
            frame = bfm_chapter_completion_draw(&p->chapter, p->save_art, p->chapter_title,
                                                p->chapter.fields, (uint32_t (*)[OPENING_W])p->pixels);
        } else if (bedroom) {
            frame = bedroom_frame(p);
        } else if (files && p->saves.page == BFM_SAVE_CARD_LOADING) {
            frame = bfm_title_frame(p->title, fields, BFM_TITLE_LOADING, BFM_CHOICE_CONTINUE,
                                    0, p->pixels, p->width, p->height);
        } else if (files) {
            bfm_save_menu_draw(p->save_art, &p->saves, fields, (uint32_t (*)[320])p->pixels);
            frame = 1;
        } else {
            frame = bfm_title_frame(p->title, fields, p->flow.phase, p->flow.choice,
                                    p->flow.prompt_visible, p->pixels, p->width, p->height);
        }
        if (p->flow.phase == BFM_TITLE_LOADING && frame == 0) {
            memset(p->pixels, 0, (size_t)p->width * p->height * sizeof *p->pixels);
            status_line(p, "Loading New Game...");
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

static int run(const char *disc_path, const char *card_path, const char *save_directory, int windowed) {
    char error[512] = {0};
    int status = 2;
    int window_width = TITLE_W, window_height = TITLE_H;
    SDL_Rect display;
    Player *p = calloc(1, sizeof *p);
    int sdl_ready = 0;
    if (!p) { fputs("Not enough memory to start.\n", stderr); return 2; }
    p->card_path = card_path;
    p->disc = musashi_disc_media_open_image(disc_path, error, sizeof error);
    if (!p->disc) {
        fprintf(stderr, "could not start from your disc image: %s\n", error);
        free(p);
        return 2;
    }
    if (!bfm_title_load(p->disc, &p->title, error, sizeof error)) goto done;
    if (save_directory) {
        const char *issue;
        p->save_copy = bfm_inn_profile_create(card_path, save_directory, &issue);
        if (!p->save_copy) {
            snprintf(error, sizeof error, "%s", issue);
            goto done;
        }
    }
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) goto sdl_error;
    sdl_ready = 1;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Dialogue will continue without sound: %s", SDL_GetError());
    p->voice = bfm_desktop_voice_open(p->disc);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    /* A bare X11 server has no window manager to resize the desktop window. */
    if (!windowed && SDL_GetDisplayBounds(0, &display) == 0 && display.w > 0 && display.h > 0) {
        window_width = display.w;
        window_height = display.h;
    }
    p->window = SDL_CreateWindow("Brave Fencer Musashi", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, window_width, window_height,
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
    bfm_forest_walk_free(p->forest);
    if (p->opening) bfm_opening_free(p->opening);
    bfm_bedroom_free(p->bedroom);
    bfm_inn_profile_close(p->save_copy);
    bfm_chapter_card_free(p->chapter_title);
    free(p->save_art);
    if (p->title) bfm_title_free(p->title);
    musashi_disc_media_close(p->disc);
    free(p);
    return status;
}

int main(int argc, char **argv) {
    const char *disc = NULL, *card = NULL, *save_directory = NULL;
    int i, windowed = 0;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            fputs(usage, stdout);
            return 0;
        }
        if (!strcmp(argv[i], "--windowed")) windowed = 1;
        else if (!strcmp(argv[i], "--save-copy") && i + 1 < argc && !save_directory) save_directory = argv[++i];
        else if (!strcmp(argv[i], "--card") && i + 1 < argc && !card) card = argv[++i];
        else if (!strcmp(argv[i], "--disc") && i + 1 < argc && !disc) disc = argv[++i];
        else if (argv[i][0] != '-' && !disc) disc = argv[i];
        else { fputs(usage, stderr); return 2; }
    }
    if (!disc) disc = getenv("BFM_DISC");
    if (!disc || !*disc) { fputs(usage, stderr); return 2; }
    if (save_directory && (!card || !*card || !*save_directory)) { fputs(usage, stderr); return 2; }
    return run(disc, card, save_directory, windowed);
}
