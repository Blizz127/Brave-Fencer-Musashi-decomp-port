#include "contact_coordinates.h"

FusionWord func_8014F3E8(Action21Actor *actor)
{
    if (twinpeak_coordinate_flags(actor) & 0x40000u) {
        return 0;
    }
    if (twinpeak_coordinate(actor, TWINPEAK_CURRENT_X) ==
        twinpeak_coordinate(actor, TWINPEAK_SAVED_X) &&
        twinpeak_coordinate(actor, TWINPEAK_CURRENT_Y) ==
        twinpeak_coordinate(actor, TWINPEAK_SAVED_Y) &&
        twinpeak_coordinate(actor, TWINPEAK_CURRENT_Z) ==
        twinpeak_coordinate(actor, TWINPEAK_SAVED_Z)) {
        return func_8014F468(actor);
    }
    return func_8014F6F4(actor);
}
