#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <assert.h>
#include "musashi_host_gamepad.h"
#include "musashi_sio_controller.h"

static uint64_t owner(void *ctx) { (void)ctx; return 1; }
static int irq(void *ctx, uint16_t bits) { (void)ctx; (void)bits; return 1; }

static int sample(void *ctx, uint16_t *buttons) {
    (void)ctx;
    *buttons = (uint16_t)~musashi_host_keyboard_sample();
    return 1;
}

static void complete(void *ctx) { (void)ctx; musashi_host_keyboard_complete(); }

static void key(SDL_Scancode sc, Uint32 type, Uint8 repeat) {
    SDL_Event e = {0};
    e.type = type;
    e.key.keysym.scancode = sc;
    e.key.repeat = repeat;
    assert(SDL_PushEvent(&e) == 1);
}

static void focus(Uint8 event) {
    SDL_Event e = {0};
    e.type = SDL_WINDOWEVENT;
    e.window.event = event;
    assert(SDL_PushEvent(&e) == 1);
}

static uint8_t transfer(MusashiSioController *sio, uint8_t data) {
    uint8_t response;
    assert(musashi_sio_controller_write8(sio, 0x1f801040, data));
    assert(musashi_sio_controller_read8(sio, 0x1f801040, &response));
    return response;
}

static uint8_t start(MusashiSioController *sio, uint8_t command) {
    assert(musashi_sio_controller_write16(sio, 0x1f80104a, 0));
    assert(musashi_sio_controller_write16(sio, 0x1f80104a, 0x1003));
    assert(transfer(sio, 1) == 0xff);
    return transfer(sio, command);
}

static uint16_t finish(MusashiSioController *sio, uint8_t id) {
    assert(transfer(sio, 0) == 0x5a);
    uint16_t buttons = transfer(sio, 0);
    buttons |= (uint16_t)(transfer(sio, 0) << 8);
    if (id != 0x41)
        for (unsigned i = 0; i < 4; ++i) (void)transfer(sio, 0);
    return buttons;
}

static uint16_t poll(MusashiSioController *sio) {
    uint8_t id = start(sio, 0x42);
    assert(id == 0x41 || id == 0x73);
    return finish(sio, id);
}

int main(void) {
    MusashiSioController sio;
    const MusashiSioPadDevice pad = {NULL, owner, sample, irq};
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0);
    musashi_host_keyboard_open();
    musashi_sio_controller_init_disconnected(&sio);
    assert(musashi_sio_controller_bind_digital_pad(&sio, &pad));
    sio.poll_complete = complete;
    assert(musashi_sio_controller_write16(&sio, 0x1f801048, 0x0d));
    assert(musashi_sio_controller_write16(&sio, 0x1f80104e, 0x88));
    assert(poll(&sio) == 0xffff);

    /* The window and other consumers may drain all SDL events between polls. */
    key(SDL_SCANCODE_C, SDL_KEYDOWN, 0);
    key(SDL_SCANCODE_C, SDL_KEYUP, 0);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {}
    assert(start(&sio, 0x42) == 0x41);
    assert(transfer(&sio, 0) == 0x5a); /* Abandon a digital poll. */
    assert(start(&sio, 0x43) == 0xf3);
    (void)finish(&sio, 0xf3);
    for (int i = 0; i < 2; ++i) {
        assert(start(&sio, 0x42) == 0xf3);
        (void)finish(&sio, 0xf3);
    }
    assert(start(&sio, 0x42) == 0x73);
    /* The host clocks a whole frame, but the guest never reads the button
     * bytes. A reset abandons those replies without accepting the tap. */
    for (int i = 0; i < 7; ++i)
        assert(musashi_sio_controller_write8(&sio, 0x1f801040, 0));
    assert(musashi_sio_controller_write16(&sio, 0x1f80104a, 0x40));
    assert(poll(&sio) == 0xbfff);
    assert(poll(&sio) == 0xffff);
    assert(poll(&sio) == 0xffff);

    /* Aliases, repeats and a second tap preserve the button transition order. */
    key(SDL_SCANCODE_RIGHT, SDL_KEYDOWN, 0);
    key(SDL_SCANCODE_RIGHT, SDL_KEYDOWN, 1);
    key(SDL_SCANCODE_D, SDL_KEYDOWN, 0);
    key(SDL_SCANCODE_RIGHT, SDL_KEYUP, 0);
    assert(poll(&sio) == 0xffdf);
    assert(poll(&sio) == 0xffdf);
    key(SDL_SCANCODE_D, SDL_KEYUP, 0);
    key(SDL_SCANCODE_D, SDL_KEYDOWN, 0);
    key(SDL_SCANCODE_D, SDL_KEYUP, 0);
    assert(poll(&sio) == 0xffff);
    assert(poll(&sio) == 0xffdf);
    assert(poll(&sio) == 0xffff);

    /* An edge arriving after selection belongs to the next sample. */
    assert(start(&sio, 0x42) == 0x73);
    key(SDL_SCANCODE_C, SDL_KEYDOWN, 0);
    key(SDL_SCANCODE_C, SDL_KEYUP, 0);
    assert(finish(&sio, 0x73) == 0xffff);
    assert(poll(&sio) == 0xbfff);
    assert(poll(&sio) == 0xffff);
    key(SDL_SCANCODE_C, SDL_KEYDOWN, 0);
    focus(SDL_WINDOWEVENT_FOCUS_LOST);
    key(SDL_SCANCODE_RIGHT, SDL_KEYDOWN, 0);
    focus(SDL_WINDOWEVENT_FOCUS_GAINED);
    key(SDL_SCANCODE_RIGHT, SDL_KEYUP, 0);
    assert(poll(&sio) == 0xffff);
    key(SDL_SCANCODE_C, SDL_KEYDOWN, 0);
    musashi_host_keyboard_suppress(1);
    key(SDL_SCANCODE_C, SDL_KEYUP, 0);
    key(SDL_SCANCODE_C, SDL_KEYDOWN, 0);
    assert(poll(&sio) == 0xffff);
    musashi_host_keyboard_suppress(0);
    key(SDL_SCANCODE_C, SDL_KEYUP, 0);
    assert(poll(&sio) == 0xffff);

    /* A direction held continuously across a modal still drives movement. */
    key(SDL_SCANCODE_RIGHT, SDL_KEYDOWN, 0);
    assert(poll(&sio) == 0xffdf);
    musashi_host_keyboard_suppress(1);
    assert(poll(&sio) == 0xffff);
    musashi_host_keyboard_suppress(0);
    assert(poll(&sio) == 0xffdf);
    key(SDL_SCANCODE_RIGHT, SDL_KEYUP, 0);
    assert(poll(&sio) == 0xffff);
    musashi_host_keyboard_close();
    SDL_Quit();
    return 0;
}
