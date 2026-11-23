#include "contact_coordinates.h"

FusionWord func_8014CC28(Action21Actor *actor)
{
    if (twinpeak_coordinate_flags(actor) & 0x80000000u) {
        if (twinpeak_coordinate(actor, TWINPEAK_CURRENT_Y) !=
            twinpeak_coordinate(actor, TWINPEAK_SAVED_Y)) {
            return func_8014D3E0(actor);
        }
        if (twinpeak_coordinate(actor, TWINPEAK_CURRENT_X) !=
            twinpeak_coordinate(actor, TWINPEAK_SAVED_X) ||
            twinpeak_coordinate(actor, TWINPEAK_CURRENT_Z) !=
            twinpeak_coordinate(actor, TWINPEAK_SAVED_Z)) {
            return func_8014D04C(actor);
        }
    }
    return func_8014CCB4(actor);
}
