/* SPDX-License-Identifier: MIT */
#ifndef BFM_VILLAGE_MINE_SHAFT_AUDIO_H
#define BFM_VILLAGE_MINE_SHAFT_AUDIO_H
#include "bedroom_effects.h"

enum { BFM_MINE_FAN_START = 0x720, BFM_MINE_FAN_DECELERATE = 0x721 };
BfmBedroomEffects *bfm_mine_shaft_effects_load(const MusashiDiscMedia *disc);

/* Encode the shaft's zero-argument effects and its targeted fan key-off
 * for the room's existing sound event queue. */
static inline unsigned bfm_mine_shaft_sound_event(unsigned command, unsigned value)
{
    if (command == 4 && value == BFM_MINE_FAN_START)
        return 0x10000u | BFM_MINE_FAN_START;
    if (value) return 0;
    switch (command) {
    case BFM_MINE_FAN_START: case BFM_MINE_FAN_DECELERATE:
    case BFM_WOODS_BOX_OPEN_SOUND: case BFM_WOODS_BOX_CLOSE_SOUND: case BFM_WOODS_BOX_ACCEPT_SOUND:
    case BFM_ROOM_MOVE_SOUND: case BFM_ROOM_ACCEPT_SOUND:
    case BFM_WALL_CLIMB_FIRST_SOUND: case BFM_WALL_CLIMB_SECOND_SOUND:
        return command;
    default:
        return 0;
    }
}
#endif
