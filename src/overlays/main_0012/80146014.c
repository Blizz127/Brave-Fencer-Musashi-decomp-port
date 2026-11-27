#include "psx_types.h"

extern s32 func_80059A80(u32 *source_rect, s32 dest_x, s32 dest_y);

/* Each copy starts with a four-halfword source rectangle. The two words
 * that follow give the destination. A zero destination row ends the list. */
void func_80146014(u32 *copy)
{
    while (copy[3] != 0) {
        func_80059A80(copy, copy[2], copy[3]);
        copy += 4;
    }
}
