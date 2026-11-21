#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "musashi_dev_menu.h"
#include "musashi_host_gamepad.h"

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "line %d failed: %s (%s)\n", __LINE__, #expr, SDL_GetError()); \
    exit(1); \
} } while (0)

#if SDL_VERSION_ATLEAST(2, 0, 14)
static int attach_pad(SDL_Joystick **joystick, int pov) {
    int index = SDL_JoystickAttachVirtual(pov ? SDL_JOYSTICK_TYPE_UNKNOWN : SDL_JOYSTICK_TYPE_GAMECONTROLLER,
            SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, pov);
    CHECK(index >= 0);
    {
        char guid[33], mapping[192];
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, sizeof guid);
        if (pov)
            snprintf(mapping, sizeof mapping, "%s,Virtual POV pad,a:b0,b:b1,start:b6,dpup:h0.1,leftx:a0,", guid);
        else
            snprintf(mapping, sizeof mapping, "%s,Virtual pad,a:b0,b:b1,guide:b5,touchpad:b20,", guid);
        CHECK(SDL_GameControllerAddMapping(mapping) >= 0);
    }
    CHECK(SDL_IsGameController(index));
    *joystick = SDL_JoystickOpen(index);
    CHECK(*joystick);
    return index;
}

static void button(SDL_Joystick *pad, SDL_GameControllerButton button, int down) {
    CHECK(SDL_JoystickSetVirtualButton(pad, button, (Uint8)down) == 0);
    SDL_GameControllerUpdate();
}

static void check_background_button(SDL_Joystick *first, SDL_Joystick *second,
                                    SDL_GameControllerButton held, unsigned alone) {
    /* Last choose the first pad, then leave only the background control down. */
    button(first, SDL_CONTROLLER_BUTTON_B, 1);
    CHECK(musashi_host_gamepad_sample().buttons == (uint16_t)~(1u << 13));
    button(first, held, 1);
    button(first, SDL_CONTROLLER_BUTTON_B, 0);
    CHECK(musashi_host_gamepad_menu_keys() == alone);
    button(second, SDL_CONTROLLER_BUTTON_A, 1);
    CHECK(musashi_host_gamepad_sample().buttons == (uint16_t)~(1u << 14));
    CHECK(musashi_host_gamepad_menu_keys() == MUSASHI_DEV_KEY_OK);
    button(first, held, 0);
    button(second, SDL_CONTROLLER_BUTTON_A, 0);
}
#endif

int main(void) {
#if SDL_VERSION_ATLEAST(2, 0, 14)
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    CHECK(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    if (SDL_NumJoysticks() != 0) {
        puts("SKIP: virtual test requires an isolated joystick session");
        SDL_Quit();
        return 77;
    }
    CHECK(musashi_host_gamepad_sample().buttons == 0xffff);
    SDL_Joystick *first, *second;
    int first_index = attach_pad(&first, 0);
    CHECK(musashi_host_gamepad_menu_keys() == 0);
    for (int i = 0; i < 65; ++i)
        CHECK(musashi_host_gamepad_sample().buttons == 0xffff);
    attach_pad(&second, 1);
    SDL_GameController *first_controller = SDL_GameControllerOpen(first_index);
    CHECK(first_controller);
    button(first, SDL_CONTROLLER_BUTTON_TOUCHPAD, 1);
    CHECK(SDL_GameControllerGetButton(first_controller, SDL_CONTROLLER_BUTTON_TOUCHPAD));
    button(first, SDL_CONTROLLER_BUTTON_TOUCHPAD, 0);
    SDL_GameControllerClose(first_controller);
    check_background_button(first, second, SDL_CONTROLLER_BUTTON_TOUCHPAD, 0);
    check_background_button(first, second, SDL_CONTROLLER_BUTTON_GUIDE, MUSASHI_DEV_KEY_TOGGLE);


    /* First pad stays attached and idle. The second drives the player's
     * Cross button, D-pad, analog position and the menu's OK key. */
    button(second, SDL_CONTROLLER_BUTTON_A, 1);
    CHECK(SDL_JoystickSetVirtualHat(second, 0, SDL_HAT_UP) == 0);
    CHECK(SDL_JoystickSetVirtualAxis(second, SDL_CONTROLLER_AXIS_LEFTX, 20480) == 0);
    SDL_GameControllerUpdate();
    MusashiHostGamepad sample = musashi_host_gamepad_sample();
    CHECK(sample.buttons == (uint16_t)~((1u << 14) | (1u << 4) | (1u << 5)));
    CHECK(sample.lx == 208 && sample.ly == 128 && sample.rx == 128 && sample.ry == 128);
    CHECK(musashi_host_gamepad_menu_keys() == (MUSASHI_DEV_KEY_OK | MUSASHI_DEV_KEY_UP));

    /* Two button presses on different pads keep the current player. */
    button(first, SDL_CONTROLLER_BUTTON_B, 1);
    CHECK(musashi_host_gamepad_menu_keys() == (MUSASHI_DEV_KEY_OK | MUSASHI_DEV_KEY_UP));
    button(second, SDL_CONTROLLER_BUTTON_A, 0);
    CHECK(SDL_JoystickSetVirtualHat(second, 0, SDL_HAT_CENTERED) == 0);
    SDL_GameControllerUpdate();
    CHECK(musashi_host_gamepad_menu_keys() == MUSASHI_DEV_KEY_BACK);
    CHECK(musashi_host_gamepad_sample().buttons == (uint16_t)~(1u << 13));

    /* Held activity on the other pad survives unplugging the active one,
     * even when the new SDL device index changes and the total stays equal. */
    button(second, SDL_CONTROLLER_BUTTON_START, 1);
    CHECK(SDL_JoystickDetachVirtual(first_index) == 0);
    SDL_JoystickClose(first);
    SDL_Joystick *replacement;
    int replacement_index = attach_pad(&replacement, 0);
    CHECK(musashi_host_gamepad_sample().buttons == (uint16_t)~((1u << 3) | (1u << 5)));
    CHECK(musashi_host_gamepad_menu_keys() == 0);
    button(second, SDL_CONTROLLER_BUTTON_START, 0);
    button(replacement, SDL_CONTROLLER_BUTTON_B, 1);
    CHECK(musashi_host_gamepad_menu_keys() == MUSASHI_DEV_KEY_BACK);
    CHECK(SDL_JoystickDetachVirtual(replacement_index) == 0);
    SDL_JoystickClose(replacement);
    sample = musashi_host_gamepad_sample();
    CHECK(sample.buttons == (uint16_t)~(1u << 5) && sample.lx == 208);
    CHECK(SDL_JoystickDetachVirtual(0) == 0); /* The second pad is now the only one. */
    SDL_JoystickClose(second);
    sample = musashi_host_gamepad_sample();
    CHECK(sample.buttons == 0xffff && sample.lx == 128 && sample.ly == 128);
    CHECK(musashi_host_gamepad_menu_keys() == 0);
    musashi_host_gamepad_close();
    SDL_Quit();
    puts("PASS two SDL virtual pads: gameplay, menu, hotplug, handoff, removal");
    return 0;
#else
    puts("SKIP: SDL virtual joysticks require SDL 2.0.14");
    return 77;
#endif
}
