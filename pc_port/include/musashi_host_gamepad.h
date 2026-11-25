#ifndef MUSASHI_HOST_GAMEPAD_H
#define MUSASHI_HOST_GAMEPAD_H

#include <stdint.h>

/* The SIO pad uses active-low buttons and centered unsigned stick coordinates. */
typedef struct {
    uint16_t buttons;
    uint8_t lx, ly, rx, ry;
} MusashiHostGamepad;

/* The SDL window owns the event queue. A normal completed pad report
 * acknowledges the keyboard state previewed by the most recent sample. */
void musashi_host_keyboard_open(void);
void musashi_host_keyboard_close(void);
void musashi_host_keyboard_suppress(int suppress);
uint16_t musashi_host_keyboard_sample(void); /* active-high */
void musashi_host_keyboard_complete(void);

MusashiHostGamepad musashi_host_gamepad_sample(void);
unsigned musashi_host_gamepad_menu_keys(void);
void musashi_host_gamepad_close(void);

#endif
