/* SPDX-License-Identifier: MIT */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "scene_views.h"
#include "desktop_voice.h"
#include "bfm_plat_font.h"
#include "title_flow.h"
#include "save_menu.h"
#include "chapter_completion_view.h"
#include "chapter_restart.h"
#include "inn_profile.h"
#include "inn_menu.h"
#include "inn_field.h"
#include "forest_walk.h"
#include "gameplay_run.h"
#include "field_waypoint.h"
#include "bedroom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { TITLE_W = 640, TITLE_H = 480, OPENING_W = 320, OPENING_H = 240 };
/* On the US disc, the first visible loading sprite spans at least 115 fields. */
enum { LOADING_VISIBLE_FIELDS = 115, CARD_READ_VISIBLE_FIELDS = 90 };
enum { PAD_SELECT = 1u, PAD_START = 1u << 3, PAD_UP = 1u << 4, PAD_RIGHT = 1u << 5, PAD_DOWN = 1u << 6,
       PAD_LEFT = 1u << 7, PAD_L2 = 1u << 8, PAD_R2 = 1u << 9, PAD_TRIANGLE = 1u << 12,
       PAD_CIRCLE = BFM_FOREST_CIRCLE, PAD_CROSS = BFM_FOREST_CROSS, PAD_SQUARE = BFM_FOREST_SQUARE };

typedef struct DesktopController {
    SDL_GameController *handle;
    SDL_JoystickID id;
    uint16_t buttons;
    struct DesktopController *next;
} DesktopController;

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
    DesktopController *controllers, *controller;
    unsigned char keys[SDL_NUM_SCANCODES];
    uint16_t digital, held, pressed, keyboard_pressed;
    int axis_x, axis_y, axis_l2, axis_r2;
    const char *card_path;
    BfmInnProfile *save_copy;
    BfmInnMenu inn_menu;
    unsigned inn_age;
    int inn_service;
    int chapter_restart_failed;
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
    "Usage: musashi_native_boot --disc path/to/game.cue [--card path/to/card.mcd] [--save-copy new-directory] [--windowed]\n"
    "Use your own US PlayStation disc image (SLUS-00726). A complete raw BIN or "
    "a CUE with all its BIN tracks alongside is supported.\n"
    "Continue reads a 128 KiB card and supports the castle wake and Grillin inn saves.\n"
    "--save-copy creates a writable memory card in a new directory. It starts blank unless you supply a read-only --card.\n"
    "Enter/Start: start. Space/Cross: confirm or skip the opening. Arrows: select. "
    "D-pad, arrows or left stick: move in Giant Forest. A/Square: swing Fusion on the ground. "
    "Right Shift/R1: charge Fusion. Z/Circle: fire earned Gunshot. Q/L2 and E/R2: rotate the camera in Lower Grillin'. "
    "In Lower Grillin' Village, Tab/Select switches to the crescent. Hold Right Shift/R1 until full, "
    "then release to sleep. Space/Cross wakes Musashi. T/Triangle: back from files. Escape: quit.\n";

static uint16_t button_mask(unsigned int button) {
    switch (button) {
    case SDL_CONTROLLER_BUTTON_BACK: return PAD_SELECT;
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

static uint16_t keyboard_directions(const Player *p) {
    uint16_t pad = 0;
    if (p->keys[SDL_SCANCODE_UP]) pad |= PAD_UP;
    if (p->keys[SDL_SCANCODE_DOWN]) pad |= PAD_DOWN;
    if (p->keys[SDL_SCANCODE_LEFT]) pad |= PAD_LEFT;
    if (p->keys[SDL_SCANCODE_RIGHT]) pad |= PAD_RIGHT;
    return pad;
}

static int charge_axis(int analog) {
    /* The charge action reads a filtered eight-bit console axis. */
    int wire = (analog + 32768) / 256;
    if (wire < 8) return -128;
    if (wire >= 248) return 127;
    if (wire > 80 && wire < 176) return 0;
    return wire - 128;
}

static void sample_field_axes(const Player *p, BfmFieldInput *field) {
    uint16_t pad = keyboard_directions(p) | (p->digital & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT));
    field->x = field_axis(pad & PAD_LEFT, pad & PAD_RIGHT, p->axis_x);
    field->y = field_axis(pad & PAD_UP, pad & PAD_DOWN, p->axis_y);
    BfmChargeDirection charge = {.sampled = 1};
    if (pad) {
        charge.x = 127 * (!!(pad & PAD_RIGHT) - !!(pad & PAD_LEFT));
        charge.y = 127 * (!!(pad & PAD_DOWN) - !!(pad & PAD_UP));
        /* The digital direction query rejects Up with both horizontal directions. */
        if ((pad & (PAD_UP | PAD_LEFT | PAD_RIGHT)) == (PAD_UP | PAD_LEFT | PAD_RIGHT)) charge.y = 0;
    } else {
        charge.x = charge_axis(p->axis_x);
        charge.y = charge_axis(p->axis_y);
    }
    field->charge_direction = charge;
    /* A D-pad direction selects the digital movement handler even when the
     * pad also has sticks. Keyboard arrows provide the same directional bits. */
    field->movement_pad = BFM_FIELD_PAD_DIGITAL;
    if (!pad && p->controller) {
        SDL_GameController *handle = p->controller->handle;
        SDL_GameControllerButtonBind x = SDL_GameControllerGetBindForAxis(handle, SDL_CONTROLLER_AXIS_LEFTX);
        SDL_GameControllerButtonBind y = SDL_GameControllerGetBindForAxis(handle, SDL_CONTROLLER_AXIS_LEFTY);
        if (x.bindType == SDL_CONTROLLER_BINDTYPE_AXIS && y.bindType == SDL_CONTROLLER_BINDTYPE_AXIS)
            field->movement_pad = BFM_FIELD_PAD_ANALOG;
    }
}

static uint16_t controller_mask(const Player *p) {
    uint16_t current = p->digital;
    if (p->axis_l2 > 16000) current |= PAD_L2;
    if (p->axis_r2 > 16000) current |= PAD_R2;
    if (p->axis_y < -16384) current |= PAD_UP;
    if (p->axis_y > 16384) current |= PAD_DOWN;
    if (p->axis_x < -16384) current |= PAD_LEFT;
    if (p->axis_x > 16384) current |= PAD_RIGHT;
    return current;
}

static void clear_presses(Player *p) {
    p->pressed = p->keyboard_pressed = 0;
}

static void update_input(Player *p, int keyboard_event) {
    uint16_t current = controller_mask(p) | keyboard_directions(p);
    if (p->keys[SDL_SCANCODE_Q]) current |= PAD_L2;
    if (p->keys[SDL_SCANCODE_E]) current |= PAD_R2;
    if (p->keys[SDL_SCANCODE_RSHIFT]) current |= BFM_FOREST_R1;
    if (p->keys[SDL_SCANCODE_TAB]) current |= PAD_SELECT;
    if (p->keys[SDL_SCANCODE_RETURN] || p->keys[SDL_SCANCODE_KP_ENTER]) current |= PAD_START;
    if (p->keys[SDL_SCANCODE_SPACE] || p->keys[SDL_SCANCODE_X]) current |= PAD_CROSS;
    if (p->keys[SDL_SCANCODE_Z] || p->keys[SDL_SCANCODE_BACKSPACE]) current |= PAD_CIRCLE;
    if (p->keys[SDL_SCANCODE_T]) current |= PAD_TRIANGLE;
    if (p->keys[SDL_SCANCODE_A]) current |= PAD_SQUARE;
    uint16_t edges = current & (uint16_t)~p->held;
    p->pressed |= edges;
    if (keyboard_event) p->keyboard_pressed |= edges;
    p->held = current;
}

static DesktopController *controller_for_id(Player *p, SDL_JoystickID id) {
    for (DesktopController *pad = p->controllers; pad; pad = pad->next)
        if (pad->id == id) return pad;
    return NULL;
}

static void add_controller(Player *p, int index, int startup) {
    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(index);
    if (id < 0 || !SDL_IsGameController(index) || controller_for_id(p, id)) return;
    SDL_GameController *handle = SDL_GameControllerOpen(index);
    if (!handle) return;
    DesktopController *pad = calloc(1, sizeof *pad);
    if (!pad) {
        SDL_GameControllerClose(handle);
        return;
    }
    pad->handle = handle;
    pad->id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(handle));
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        if (SDL_GameControllerGetButton(handle, (SDL_GameControllerButton)i))
            pad->buttons |= button_mask((unsigned)i);
    }
    pad->next = p->controllers;
    p->controllers = pad;
    if (p->controller) return;
    p->controller = pad;
    if (startup) {
        p->digital = pad->buttons;
        p->axis_x = SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTX);
        p->axis_y = SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTY);
        p->axis_l2 = SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        p->axis_r2 = SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    }
}

static void find_controller(Player *p) {
    int n = SDL_NumJoysticks();
    for (int i = 0; i < n; ++i) add_controller(p, i, 1);
    update_input(p, 0);
}

static void reset_controller_input(Player *p) {
    p->digital = 0;
    p->axis_x = p->axis_y = p->axis_l2 = p->axis_r2 = 0;
}

static void remove_controller(Player *p, SDL_JoystickID id) {
    DesktopController **link = &p->controllers;
    while (*link && (*link)->id != id) link = &(*link)->next;
    DesktopController *pad = *link;
    if (!pad) return;
    *link = pad->next;
    if (p->controller == pad) {
        p->controller = p->controllers;
        reset_controller_input(p);
        p->pressed = p->keyboard_pressed;
    }
    SDL_GameControllerClose(pad->handle);
    free(pad);
}

static void close_controllers(Player *p) {
    while (p->controllers) remove_controller(p, p->controllers->id);
}

static void controller_button(Player *p, const SDL_ControllerButtonEvent *event) {
    DesktopController *pad = controller_for_id(p, event->which);
    uint16_t mask = button_mask(event->button);
    if (!pad || !mask) return;
    if (event->type == SDL_CONTROLLERBUTTONDOWN) {
        if (pad->buttons & mask) return;
        pad->buttons |= mask;
        if (p->controller != pad) {
            /* A fresh mapped button picks up an idle player's controls. */
            if (controller_mask(p)) return;
            if (p->flow.phase == BFM_TITLE_BEDROOM && !p->inn_service &&
                bfm_pad_magnitude(field_axis(0, 0, p->axis_x), field_axis(0, 0, p->axis_y)) >= 16)
                return;
            p->controller = pad;
            reset_controller_input(p);
        }
        p->digital |= mask;
    } else {
        pad->buttons &= (uint16_t)~mask;
        if (pad == p->controller) p->digital &= (uint16_t)~mask;
    }
}

static const char *bedroom_caption(uint16_t room)
{
    switch (room) {
    case 0x3001: return "Brave Fencer Musashi - Castle bedroom";
    case 0x3002: return "Brave Fencer Musashi - Castle library";
    case 0x3003: return "Brave Fencer Musashi - Castle audience";
    case 0x3000: return "Brave Fencer Musashi - Castle courtyard";
    case 0x3004: return "Brave Fencer Musashi - Castle cable station";
    case 0x1011: return "Brave Fencer Musashi - Upper Grillin' Village";
    case 0x1052:
    case 0x1010: return "Brave Fencer Musashi - Lower Grillin' Village";
    case 0x2058: return "Brave Fencer Musashi - Hotelo's inn (Motelo)";
    case 0x2017: return "Brave Fencer Musashi - Hotelo's inn";
    case 0x3014: return "Brave Fencer Musashi - Forest bank";
    case 0x3025: return "Brave Fencer Musashi - Southern woods";
    case 0x3012: return "Brave Fencer Musashi - Recovery room";
    default: return "Brave Fencer Musashi - Gameplay";
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
        p->gameplay.saved_data.present = p->gameplay.memory_box.present = 0;
        memset(p->gameplay.story_flags, 0, sizeof p->gameplay.story_flags);
        caption = "Brave Fencer Musashi - Loading New Game";
        break;
    case BFM_TITLE_OPENING: caption = "Brave Fencer Musashi - Opening"; break;
    case BFM_TITLE_CHAPTER_COMPLETION: caption = "Brave Fencer Musashi - Chapter complete"; break;
    case BFM_TITLE_BEDROOM:
        p->display_room = bfm_bedroom_display_room(p->bedroom);
        caption = bedroom_caption(p->display_room);
        break;
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
                clear_presses(p);
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            add_controller(p, e.cdevice.which, 0);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            remove_controller(p, e.cdevice.which);
            break;
        case SDL_CONTROLLERAXISMOTION:
            if (p->controller && e.caxis.which == p->controller->id) {
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) p->axis_y = e.caxis.value;
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) p->axis_x = e.caxis.value;
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) p->axis_l2 = e.caxis.value;
                if (e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) p->axis_r2 = e.caxis.value;
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            controller_button(p, &e.cbutton);
            break;
        }
        update_input(p, e.type == SDL_KEYDOWN || e.type == SDL_KEYUP);
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
    uint16_t room = bfm_bedroom_display_room(p->bedroom);
    if (room != p->display_room) {
        SDL_SetWindowTitle(p->window, bedroom_caption(room));
        p->display_room = room;
    }
    if (p->chapter_restart_failed)
        status_line(p, "Could not load the castle bedroom. Press Start to retry.");
    else if (bfm_bedroom_endpoint(p->bedroom))
        status_line(p, bfm_bedroom_end_message(p->bedroom));
    return 1;
}

static const BfmSaveRecord *read_keeper_card(Player *p)
{
    if (!p->save_copy) return NULL;
    const BfmSaveRecord *recovered = bfm_inn_profile_menu(p->save_copy, &p->saves);
    if (recovered) bfm_gameplay_remember_saved(&p->gameplay, recovered);
    return recovered;
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
        read_keeper_card(p);
        bfm_inn_menu_begin(&p->inn_menu, p->save_copy ? &p->saves : NULL);
        BfmInnField *inn = bfm_bedroom_inn_counter(p->bedroom);
        p->inn_menu.field_palette = bfm_inn_field_menu_palette(inn);
        for (unsigned i = 0; i < 6; ++i)
            p->inn_menu.prices[i] = bfm_inn_room_price(inn, i);
        p->inn_service = 1;
        p->inn_age = 0;
        return 1;
    }
    ++p->inn_age;
    if (p->inn_menu.page == BFM_INN_WRITING) {
        unsigned file = p->inn_menu.card.selected + 1;
        if (read_keeper_card(p)) {
            /* Time advances at the counter. Keep the earlier verified record,
             * even when the new request targets its now-occupied slot. */
            p->inn_menu.card = p->saves;
            p->inn_menu.card.selected = file - 1;
            if (p->saves.selected == file - 1 || p->inn_menu.overwriting) {
                p->inn_menu.page = BFM_INN_FILE_ERROR;
                p->inn_menu.notice = "Earlier save verified. Select a file.";
                return 1;
            }
        }
        const char *error = p->save_copy ? p->saves.issue : NULL;
        if (!error) {
            if (p->inn_menu.overwriting)
                error = bfm_inn_profile_save_selected(p->save_copy, &p->inn_menu.card, record, p->disc);
            else
                error = bfm_inn_profile_save(p->save_copy, file, record, p->disc);
        }
        if (!error) bfm_inn_profile_menu(p->save_copy, &p->saves);
        bfm_inn_menu_complete(&p->inn_menu, error ? NULL : &p->saves, error);
        if (p->inn_menu.page == BFM_INN_SAVED)
            bfm_gameplay_remember_saved(&p->gameplay,
                                  &p->saves.records[file - 1]);
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
    bfm_opening_advance_playtime(p->opening, field, &p->gameplay);
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
    clear_presses(p);
    p->forest_pressed = 0;
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
        clear_presses(p);
        BfmChapterSavePage before = p->chapter.page;
        if (!bfm_chapter_completion_step(&p->chapter, p->disc, p->save_copy, pressed)) return 0;
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
        p->gameplay.display_page ^= 1u;
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
        p->gameplay.display_page ^= 1u;
        switch (bfm_forest_walk_retry_audio(p->forest)) {
        case BFM_RETRY_AUDIO_FADE_FIELD:
            bfm_desktop_voice_defeat_score(p->voice, p->disc, BFM_DEFEAT_SCORE_FADE_FIELD);
            break;
        case BFM_RETRY_AUDIO_START:
            bfm_desktop_voice_defeat_score(p->voice, p->disc, BFM_DEFEAT_SCORE_START);
            break;
        case BFM_RETRY_AUDIO_FADE_OUT:
            bfm_desktop_voice_defeat_score(p->voice, p->disc, BFM_DEFEAT_SCORE_FADE_OUT);
            break;
        case BFM_RETRY_AUDIO_RESTORE_ROOM:
            if (bfm_forest_walk_room(p->forest) == 0x3008 ||
                (bfm_forest_walk_room(p->forest) == 0x300b &&
                 !bfm_forest_walk_retry_music_track(p->forest)))
                bfm_desktop_voice_forest_score(p->voice, p->disc, SDL_GetTicks64());
            else if (bfm_forest_walk_room(p->forest) == 0x300b)
                bfm_desktop_voice_tower_retry_music(p->voice, p->disc,
                                                    bfm_forest_walk_retry_music_track(p->forest));
            break;
        case BFM_RETRY_AUDIO_STOP_OPTICAL:
            bfm_desktop_voice_end_tower_music(p->voice);
            break;
        case BFM_RETRY_AUDIO_NONE:
            break;
        }
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
        if (bfm_forest_walk_defeated(p->forest)) {
            SDL_SetWindowTitle(p->window, "Brave Fencer Musashi");
        } else if (room == 0x300e) {
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
    clear_presses(p);
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
                    bfm_gameplay_remember_saved(&p->gameplay,
                                          &p->saves.records[p->saves.selected]);
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
                if (bfm_bedroom_retry_destination(p->bedroom) == BFM_WOODS_RESTART_CHAPTER) {
                    if (!p->chapter_restart_failed || (p->pressed & PAD_START)) {
                        BfmBedroom *castle = bfm_chapter_restart_open(p->disc, &p->gameplay);
                        p->chapter_restart_failed = castle == NULL;
                        if (castle) {
                            bfm_desktop_voice_stop(p->voice);
                            bfm_bedroom_free(p->bedroom);
                            p->bedroom = castle;
                        }
                        room_advanced = 1;
                    }
                } else if (bfm_bedroom_endpoint(p->bedroom) && (p->pressed & (PAD_TRIANGLE | PAD_START | PAD_CROSS))) {
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
                        BfmFieldInput field = {.square_pressed=scene_input & PAD_SQUARE,
                                               .pressed_directions=scene_input & (PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT),
                                               .shoulder_held=!service && (p->held & BFM_FOREST_R1),
                                               .select_pressed=scene_input & PAD_SELECT,
                                               .start_pressed=scene_input & PAD_START,
                                               .camera_turn=service ? 0 :
                                                   !!(p->held & PAD_R2) - !!(p->held & PAD_L2),
                                               .camera_pressed=scene_input & (PAD_L2 | PAD_R2),
                                               .circle_pressed=scene_input & PAD_CIRCLE,
                                               .triangle_pressed=scene_input & PAD_TRIANGLE,
                                               .cross_pressed=scene_input & PAD_CROSS,
                                               .cross_held=!service && (p->held & PAD_CROSS),
                                               .held_faces=service ? 0 : p->held &
                                                   (PAD_TRIANGLE | PAD_CIRCLE | PAD_CROSS | PAD_SQUARE)};
                        sample_field_axes(p, &field);
                        int result = bfm_bedroom_step_field(p->bedroom,
                                    scene_input & (PAD_CIRCLE | PAD_CROSS | PAD_TRIANGLE),
                                    scene_input & PAD_CIRCLE, direction, held, scene_input & PAD_CROSS,
                                    scene_input & PAD_TRIANGLE, field);
                        if (!result || !keeper_service(p, pressed, service)) {
                            snprintf(error, size, "Could not advance the mounted room.");
                            return 2;
                        }
                        if (result == BFM_BEDROOM_VOICE_STOPPED) bfm_desktop_voice_room_skip(p->voice);
                        if (bfm_bedroom_take_effect_release(p->bedroom))
                            bfm_desktop_voice_release_room_effects(p->voice);
                        bfm_desktop_voice_room_score(p->voice, bfm_bedroom_score(p->bedroom));
                        bfm_desktop_voice_room_score_command(p->voice, bfm_bedroom_take_score_command(p->bedroom));
                        BfmRetryAudioEvent retry_audio = bfm_bedroom_retry_audio(p->bedroom);
                        BfmDefeatScoreEvent score_command = BFM_DEFEAT_SCORE_START;
                        const char *score_action = NULL;
                        switch (retry_audio) {
                        case BFM_RETRY_AUDIO_FADE_FIELD:
                            score_command = BFM_DEFEAT_SCORE_FADE_FIELD;
                            score_action = "fade the field score";
                            break;
                        case BFM_RETRY_AUDIO_START:
                            score_action = "mount and start the defeat score";
                            break;
                        case BFM_RETRY_AUDIO_FADE_OUT:
                            score_command = BFM_DEFEAT_SCORE_FADE_OUT;
                            score_action = "release the defeat score";
                            break;
                        default:
                            break;
                        }
                        if (score_action) {
                            int accepted = bfm_desktop_voice_defeat_score(p->voice, p->disc, score_command);
                            if (!bfm_bedroom_retry_score_result(p->bedroom, retry_audio, accepted)) {
                                snprintf(error, size, "Could not %s during the retry.", score_action);
                                return 2;
                            }
                        }
                        int slider_event;
                        struct BfmCableSound *slider=bfm_bedroom_slider(p->bedroom,&slider_event);
                        bfm_desktop_voice_slider(p->voice,slider,slider_event);
                        bfm_desktop_voice_landing(p->voice, bfm_bedroom_landing(p->bedroom));
                        const BfmBedroomSound *sounds;
                        size_t count = bfm_bedroom_take_effects(p->bedroom, &sounds);
                        bfm_desktop_voice_room_effects(p->voice, sounds, count);
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
            /* Menus execute one input update. The timed opening catches up to
             * its playhead, while forest and room loops count their own steps. */
            if (!p->forest && p->flow.phase != BFM_TITLE_BEDROOM) {
                unsigned pages = 1;
                if ((int)p->flow.phase == before &&
                    (before == BFM_TITLE_OPENING || before == BFM_TITLE_LOADING))
                    pages = (unsigned)(tick - last_logic);
                p->gameplay.display_page ^= pages & 1u;
            }
            clear_presses(p);
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
        if (opening) bfm_opening_advance_playtime(p->opening, fields, &p->gameplay);
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
            if (!bfm_gameplay_new_player(&p->gameplay, p->disc)) {
                snprintf(error, size, "Could not load the New Game player and figure data from your disc.");
                return 2;
            }
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
    close_controllers(p);
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
    if (save_directory && (!*save_directory || (card && !*card))) { fputs(usage, stderr); return 2; }
    return run(disc, card, save_directory, windowed);
}
