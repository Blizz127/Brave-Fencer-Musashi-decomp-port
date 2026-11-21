#ifndef MUSASHI_HOST_GAMEPAD_H
#define MUSASHI_HOST_GAMEPAD_H

#include <stdint.h>

/* The SIO pad uses active-low buttons and centered unsigned stick coordinates. */
typedef struct {
    uint16_t buttons;
    uint8_t lx, ly, rx, ry;
} MusashiHostGamepad;

MusashiHostGamepad musashi_host_gamepad_sample(void);
unsigned musashi_host_gamepad_menu_keys(void);
void musashi_host_gamepad_close(void);

#endif
