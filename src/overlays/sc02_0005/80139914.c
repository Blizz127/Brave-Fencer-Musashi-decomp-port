#include "tower_request.h"

s32 func_80139914(void *request)
{
    void *attachment;
    REQUEST_CLOSE_RESULT(mode) = REQUEST_HALF(request, 0x18);
    REQUEST_CLEAR_HALF(request, 4);
    REQUEST_CLEAR_WORD(request, 0);
    if (mode) {
        attachment = REQUEST_LINK(request);
        if (attachment)
            func_8013A8B0(attachment);
    }
    return mode;
}
