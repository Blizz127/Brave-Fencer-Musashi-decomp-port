#ifndef MUSASHI_TWINPEAK_CONTACT_COORDINATES_H
#define MUSASHI_TWINPEAK_CONTACT_COORDINATES_H

#include "contact_state.h"

#if !defined(__mips__) || defined(__LP64__)
#define func_8014CCB4 bfm_sc03_0029_provider_8014ccb4
#define func_8014D04C bfm_sc03_0029_provider_8014d04c
#define func_8014D3E0 bfm_sc03_0029_provider_8014d3e0
#define func_8014F468 bfm_sc03_0029_provider_8014f468
#define func_8014F6F4 bfm_sc03_0029_provider_8014f6f4
#endif

extern FusionWord func_8014CCB4(Action21Actor *actor);
extern FusionWord func_8014D04C(Action21Actor *actor);
extern FusionWord func_8014D3E0(Action21Actor *actor);
extern FusionWord func_8014F468(Action21Actor *actor);
extern FusionWord func_8014F6F4(Action21Actor *actor);

enum TwinpeakCoordinateOffset {
    TWINPEAK_CURRENT_X = 6,
    TWINPEAK_CURRENT_Y = 10,
    TWINPEAK_CURRENT_Z = 14,
    TWINPEAK_SAVED_X = 0x88,
    TWINPEAK_SAVED_Y = 0x8a,
    TWINPEAK_SAVED_Z = 0x8c
};

static __inline__ int twinpeak_coordinate(const Action21Actor *actor,
                                          enum TwinpeakCoordinateOffset offset)
{
    const unsigned char *field = (const unsigned char *)actor + offset;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    return *(const signed short *)field;
#else
    unsigned int value = field[0] | ((unsigned int)field[1] << 8);
    return (int)(value ^ 0x8000u) - 0x8000;
#endif
}

static __inline__ FusionWord twinpeak_coordinate_flags(const Action21Actor *actor)
{
    const unsigned char *field = (const unsigned char *)actor + 0x44;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    return *(const FusionWord *)field;
#else
    return (FusionWord)field[0] | ((FusionWord)field[1] << 8) |
           ((FusionWord)field[2] << 16) | ((FusionWord)field[3] << 24);
#endif
}

#endif
