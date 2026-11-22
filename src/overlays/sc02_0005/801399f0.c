#include "tower_request.h"

s32 func_801399F0(void *request)
{
    s16 state;
    if (!REQUEST_WORD(request, 0))
        return -1;
    state = (s16)REQUEST_HALF(request, 4);
    if (state == 0 || state == 14)
        return -1;
    if (state == 13)
        return ((s8 *)request)[0x1F];
    return 0;
}
