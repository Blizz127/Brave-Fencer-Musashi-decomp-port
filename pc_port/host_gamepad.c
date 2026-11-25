#include "musashi_host_gamepad.h"
#include "musashi_sdl_keyboard.h"
#include "musashi_dev_menu.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

static MusashiSdlKeyboard keyboard;

void musashi_host_keyboard_open(void) {
    static const struct { SDL_Scancode key; unsigned bit; } mapping[] = {
        {SDL_SCANCODE_SPACE,0}, {SDL_SCANCODE_RETURN,3},
        {SDL_SCANCODE_UP,4}, {SDL_SCANCODE_RIGHT,5},
        {SDL_SCANCODE_DOWN,6}, {SDL_SCANCODE_LEFT,7},
        {SDL_SCANCODE_W,4}, {SDL_SCANCODE_D,5},
        {SDL_SCANCODE_S,6}, {SDL_SCANCODE_A,7},
        {SDL_SCANCODE_LCTRL,8}, {SDL_SCANCODE_RCTRL,9},
        {SDL_SCANCODE_LSHIFT,10}, {SDL_SCANCODE_RSHIFT,11},
        {SDL_SCANCODE_Z,12}, {SDL_SCANCODE_V,13},
        {SDL_SCANCODE_C,14}, {SDL_SCANCODE_X,15},
        {SDL_SCANCODE_U,12}, {SDL_SCANCODE_I,13},
        {SDL_SCANCODE_K,14}, {SDL_SCANCODE_J,15}
    };
    uint16_t bindings[SDL_NUM_SCANCODES] = {0};
    for (unsigned i = 0; i < sizeof mapping / sizeof mapping[0]; ++i)
        bindings[mapping[i].key] = (uint16_t)(1u << mapping[i].bit);
    musashi_sdl_keyboard_open(&keyboard, bindings);
}

void musashi_host_keyboard_close(void) {
    musashi_sdl_keyboard_close(&keyboard);
}

void musashi_host_keyboard_suppress(int suppress) {
    musashi_sdl_keyboard_suppress(&keyboard, suppress);
}

uint16_t musashi_host_keyboard_sample(void) {
    uint16_t buttons = musashi_sdl_keyboard_sample(&keyboard);
    /* Some SDL keyboard providers expose Right Shift only in the modifier
     * state. Normal key events already carry their own press/release order. */
    int n = 0;
    const Uint8 *keys = SDL_GetKeyboardState(&n);
    if (keys && n > SDL_SCANCODE_RSHIFT && !keys[SDL_SCANCODE_RSHIFT] &&
        SDL_GetKeyboardFocus() && (SDL_GetModState() & KMOD_RSHIFT)) {
        SDL_AtomicLock(&keyboard.lock);
        if (!keyboard.suppressed && !keyboard.unfocused)
            buttons |= (uint16_t)(1u << 11);
        SDL_AtomicUnlock(&keyboard.lock);
    }
    return buttons;
}

void musashi_host_keyboard_complete(void) {
    musashi_sdl_keyboard_complete(&keyboard);
}

/* Both consumers sample the same SDL controller. SDL's event queue stays
 * owned by the window. Keep references across samples so a second device can
 * become active while the first one is still attached. */
static SDL_GameController **controllers;
static int controller_count;
static int joystick_count = -1;
static Uint32 last_scan;
static SDL_GameController *selected;

void musashi_host_gamepad_close(void) {
    for (int i = 0; i < controller_count; ++i)
        SDL_GameControllerClose(controllers[i]);
    free(controllers);
    controllers = NULL;
    controller_count = 0;
    joystick_count = -1;
    selected = NULL;
}

static uint16_t game_buttons(SDL_GameController *controller);
static unsigned menu_keys(SDL_GameController *pad);

static int activity(SDL_GameController *controller, int menu) {
    if (menu) {
        unsigned keys = menu_keys(controller);
        /* A held Guide/stick chord still toggles the menu on its edge. An
         * actual navigation key on another pad can take over afterward. */
        return keys & ~MUSASHI_DEV_KEY_TOGGLE ? 2 : keys != 0;
    }
    if (game_buttons(controller) != 0xffff) return 2;
    for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
        int v = SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)i);
        int deadzone = i >= SDL_CONTROLLER_AXIS_TRIGGERLEFT ? 16000 : 3000;
        if (v > deadzone || v < -deadzone) return 1;
    }
    return 0;
}

static SDL_GameController *active_controller(int menu) {
    if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER) &&
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) return NULL;

    int total = SDL_NumJoysticks();
    Uint32 now = SDL_GetTicks();
    int rescan = total != joystick_count || now - last_scan >= 1000;
    for (int i = 0; i < controller_count; ) {
        SDL_GameController *pad = controllers[i];
        if (SDL_GameControllerGetAttached(pad)) {
            ++i;
        } else {
            if (pad == selected) selected = NULL;
            SDL_GameControllerClose(pad);
            controllers[i] = controllers[--controller_count];
            rescan = 1;
        }
    }
    if (rescan && total >= 0) {
        /* At most one handle per joystick. Refresh if a mapping is installed
         * after connection, or a previously unavailable device becomes usable. */
        if (total > controller_count) {
            SDL_GameController **grown = realloc(controllers, (size_t)total * sizeof *controllers);
            if (grown) {
                controllers = grown;
                for (int i = 0; i < total && controller_count < total; ++i) {
                    if (!SDL_IsGameController(i)) continue;
                    SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);
                    int j = 0;
                    while (j < controller_count &&
                           SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers[j])) != id)
                        ++j;
                    if (j == controller_count) {
                        SDL_GameController *pad = SDL_GameControllerOpen(i);
                        if (pad) {
                            controllers[controller_count++] = pad;
                            fprintf(stderr, "native_boot: GameController attached: %s\n",
                                    SDL_GameControllerName(pad));
                        }
                    }
                }
            }
        }
        joystick_count = total;
        last_scan = now;
    }

    if (!selected && controller_count) selected = controllers[0];
    int best = selected ? activity(selected, menu) : 0;
    for (int i = 0; i < controller_count && best < 2; ++i) {
        if (controllers[i] == selected) continue;
        int score = activity(controllers[i], menu);
        if (score > best) {
            selected = controllers[i];
            best = score;
        }
    }
    return selected;
}

static uint16_t game_buttons(SDL_GameController *controller) {
    uint16_t buttons = 0xffff;
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
        buttons &= (uint16_t)~(1u << 4);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        buttons &= (uint16_t)~(1u << 5);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
        buttons &= (uint16_t)~(1u << 6);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        buttons &= (uint16_t)~(1u << 7);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))
        buttons &= (uint16_t)~(1u << 14); /* Cross / Jump */
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X))
        buttons &= (uint16_t)~(1u << 15); /* Square / Attack */
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))
        buttons &= (uint16_t)~(1u << 13); /* Circle / Secondary */
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y))
        buttons &= (uint16_t)~(1u << 12); /* Triangle */
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START))
        buttons &= (uint16_t)~(1u << 3);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK))
        buttons &= (uint16_t)~(1u << 0);
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
        buttons &= (uint16_t)~(1u << 10); /* L1 */
    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
        buttons &= (uint16_t)~(1u << 11); /* R1 */
    return buttons;
}

MusashiHostGamepad musashi_host_gamepad_sample(void) {
    MusashiHostGamepad state = {0xffffu, 128, 128, 128, 128};
    SDL_GameController *controller = active_controller(0);
    if (!controller) return state;
    state.buttons = game_buttons(controller);
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000)
        state.buttons &= (uint16_t)~(1u << 8); /* L2 */
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000)
        state.buttons &= (uint16_t)~(1u << 9); /* R2 */

    int16_t axis_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    int16_t axis_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
    int16_t axis_rx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
    int16_t axis_ry = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);
    if (axis_x < -10000) state.buttons &= (uint16_t)~(1u << 7); /* Left */
    if (axis_x > 10000) state.buttons &= (uint16_t)~(1u << 5);  /* Right */
    if (axis_y < -10000) state.buttons &= (uint16_t)~(1u << 4); /* Up */
    if (axis_y > 10000) state.buttons &= (uint16_t)~(1u << 6);  /* Down */

    if (axis_x < -3000 || axis_x > 3000)
        state.lx = (uint8_t)(((axis_x >> 8) + 128) & 0xff);
    if (axis_y < -3000 || axis_y > 3000)
        state.ly = (uint8_t)(((axis_y >> 8) + 128) & 0xff);
    if (axis_rx < -3000 || axis_rx > 3000)
        state.rx = (uint8_t)(((axis_rx >> 8) + 128) & 0xff);
    if (axis_ry < -3000 || axis_ry > 3000)
        state.ry = (uint8_t)(((axis_ry >> 8) + 128) & 0xff);
    return state;
}

static unsigned menu_keys(SDL_GameController *pad) {
    unsigned held = 0;
    if (!pad) return 0;
    int ly = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
    if ((SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSTICK) &&
         SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK)) ||
        SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_GUIDE))
        held |= MUSASHI_DEV_KEY_TOGGLE;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) || ly < -16000)
        held |= MUSASHI_DEV_KEY_UP;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || ly > 16000)
        held |= MUSASHI_DEV_KEY_DOWN;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) held |= MUSASHI_DEV_KEY_OK;
    if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B)) held |= MUSASHI_DEV_KEY_BACK;
    return held;
}

unsigned musashi_host_gamepad_menu_keys(void) {
    return menu_keys(active_controller(1));
}
