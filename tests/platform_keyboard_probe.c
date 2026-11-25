#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <assert.h>
#include <string.h>
#include "bfm_plat.h"
#include "bfm_plat_mods.h"

static uint16_t mod_clear, mod_set;
static int mod_polls;
BfmPlatConfig *bfm_plat_config(void) { return NULL; }
const char *bfm_plat_config_get(const BfmPlatConfig *cfg, const char *key) {
    (void)cfg;
    return strcmp(key, "input.analog") == 0 ? "1" : NULL;
}
int bfm_plat_ini_bool(const char *text, int *out) {
    *out = strcmp(text, "1") == 0;
    return BFM_PLAT_OK;
}
int bfm_plat_console_ui_is_open(void) { return 0; }
void bfm_plat_mods_emit(BfmEvent event, void *payload) {
    BfmEventInput *input = payload;
    assert(event == BFM_EVENT_INPUT);
    input->pads[0].buttons = (input->pads[0].buttons & ~mod_clear) | mod_set;
    ++mod_polls;
}
int bfm_plat_backend_sdl_input_register(void);

static void tap(SDL_Scancode code) {
    SDL_Event event = {0};
    SDL_Window *focus = SDL_GetKeyboardFocus();
    event.type = SDL_KEYDOWN;
    event.key.windowID = focus ? SDL_GetWindowID(focus) : 0;
    event.key.state = SDL_PRESSED;
    event.key.keysym.scancode = code;
    assert(SDL_PushEvent(&event) == 1);
    event.type = SDL_KEYUP;
    event.key.state = SDL_RELEASED;
    assert(SDL_PushEvent(&event) == 1);
}
static uint16_t sample(void) {
    BfmPlatPad pad;
    assert(bfm_plat_input_sample_gamepad(&pad) == BFM_PLAT_OK && pad.connected);
    return pad.buttons;
}
int main(void) {
    const BfmPlatBinding bindings[] = {
        {SDL_SCANCODE_Q, 0, 0, BFM_PAD_CROSS},
        {SDL_SCANCODE_E, 1, 0, BFM_PAD_RIGHT},
        {SDL_SCANCODE_T, 0, 1, BFM_HOTKEY_PAUSE},
        {0x10000 | SDL_CONTROLLER_BUTTON_A, 0, 0, BFM_PAD_CROSS}
    };
    const char *selected;
    BfmPlatPad ordinary;
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    assert(bfm_plat_backend_sdl_input_register() == BFM_PLAT_OK);
    assert(bfm_plat_input_open("sdl", &selected) == BFM_PLAT_OK);
    assert(strcmp(selected, "sdl") == 0);
    assert(bfm_plat_input_set_bindings(bindings, sizeof bindings / sizeof *bindings) == BFM_PLAT_OK);
    bfm_plat_input_gameplay_keyboard(1);
    assert(bfm_plat_input_poll() == BFM_PLAT_OK);
    tap(SDL_SCANCODE_Q);
    assert(sample() == BFM_PAD_CROSS);
    assert(bfm_plat_input_poll() == BFM_PLAT_OK);
    assert(bfm_plat_input_pad(0, &ordinary) == BFM_PLAT_OK && ordinary.buttons == BFM_PAD_CROSS);
    assert(sample() == BFM_PAD_CROSS);
    bfm_plat_input_complete_pad_poll();
    assert(sample() == 0);
    bfm_plat_input_complete_pad_poll();
    assert(mod_polls == 2);
    tap(SDL_SCANCODE_E);
    tap(SDL_SCANCODE_T);
    assert(sample() == 0);
    mod_set = BFM_PAD_LEFT;
    mod_clear = BFM_PAD_CROSS;
    tap(SDL_SCANCODE_Q);
    assert(bfm_plat_input_poll() == BFM_PLAT_OK);
    assert(sample() == BFM_PAD_LEFT);
    bfm_plat_input_complete_pad_poll();
    assert(sample() == BFM_PAD_LEFT);
    bfm_plat_input_complete_pad_poll();
    bfm_plat_input_keyboard_modal(1);
    assert(sample() == 0);
    assert(bfm_plat_input_pad(0, &ordinary) == BFM_PLAT_OK && ordinary.buttons == BFM_PAD_LEFT);
    bfm_plat_input_keyboard_modal(0);
    mod_set = mod_clear = 0;
    assert(bfm_plat_input_poll() == BFM_PLAT_OK);
    bfm_plat_input_suppress(1);
    bfm_plat_input_keyboard_modal(1);
    tap(SDL_SCANCODE_Q);
    bfm_plat_input_suppress(0);
    tap(SDL_SCANCODE_Q);
    bfm_plat_input_keyboard_modal(0);
    assert(sample() == 0);
#if SDL_VERSION_ATLEAST(2, 0, 14)
    if (!SDL_NumJoysticks()) {
        int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
                            SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        assert(index >= 0);
        SDL_Joystick *pad = SDL_JoystickOpen(index);
        assert(pad);
        assert(SDL_JoystickSetVirtualButton(pad, SDL_CONTROLLER_BUTTON_A, 1) == 0);
        assert(SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_LEFTX, 16384) == 0);
        SDL_GameControllerUpdate();
        tap(SDL_SCANCODE_Q);
        assert(bfm_plat_input_poll() == BFM_PLAT_OK);
        assert(sample() == BFM_PAD_CROSS);
        bfm_plat_input_complete_pad_poll();
        assert(bfm_plat_input_sample_gamepad(&ordinary) == BFM_PLAT_OK);
        assert(ordinary.buttons == BFM_PAD_CROSS && ordinary.lx == 192 && ordinary.analog);
        bfm_plat_input_complete_pad_poll();
        bfm_plat_input_keyboard_modal(1);
        assert(bfm_plat_input_sample_gamepad(&ordinary) == BFM_PLAT_OK && ordinary.connected);
        assert(ordinary.buttons == 0 && ordinary.lx == 128 && ordinary.ly == 128 &&
               ordinary.rx == 128 && ordinary.ry == 128 && ordinary.analog);
        assert(bfm_plat_input_pad(0, &ordinary) == BFM_PLAT_OK);
        assert(ordinary.buttons == BFM_PAD_CROSS && ordinary.lx == 192);
        bfm_plat_input_keyboard_modal(0);
        assert(bfm_plat_input_sample_gamepad(&ordinary) == BFM_PLAT_OK);
        assert(ordinary.buttons == BFM_PAD_CROSS && ordinary.lx == 192);
        assert(SDL_JoystickDetachVirtual(index) == 0);
        SDL_JoystickClose(pad);
        assert(bfm_plat_input_poll() == BFM_PLAT_OK);
    }
#endif
    tap(SDL_SCANCODE_Q);
    assert(bfm_plat_input_set_bindings(NULL, 0) == BFM_PLAT_OK);
    assert(sample() == 0);
    bfm_plat_input_close();
    SDL_Quit();
    return 0;
}
