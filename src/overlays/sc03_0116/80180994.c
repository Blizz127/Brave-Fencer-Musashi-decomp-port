#include "room_event.h"

s32 func_80180994(Bfm304cEvent *event)
{
    return D_801862EC[event->phase](event);
}
