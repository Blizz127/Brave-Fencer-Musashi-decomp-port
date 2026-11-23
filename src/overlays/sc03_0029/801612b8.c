#include "contact_state.h"

FusionWord func_801612B8(Action21Actor *actor, unsigned int argument)
{
    FusionWord status = 0;

    if ((argument & 1) == 0) {
        func_8014E934(actor);
    }
    status |= func_8014CC28(actor);
    status |= func_8014F3E8(actor);
    return status;
}
