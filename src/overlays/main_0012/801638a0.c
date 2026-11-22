/* func_801638A0 [801638A0,80163950) - clean-room owner re-derivation (nmG). */
/* Check the movement from the saved position and classify the contacted slope. */
#include "forest_floor.h"
#ifndef __mips__
#include <string.h>
#endif

extern void func_80015978(u16 *source, u16 *destination);
extern void func_80015954(u16 *source, u16 *destination);

typedef struct { u8 bytes[8]; } SavedPointBytes;

s32 func_801638A0(u8 *actor, FloorPoint *endpoint)
{
    union {
        SavedPointBytes raw;
        FloorPoint point;
    } previous;
    u16 *position = (u16 *)(actor + 4);

#ifdef __mips__
    /* The original copies the eight bytes without assuming word alignment. */
    previous.raw = *(const SavedPointBytes *)(actor + 0x44);
#else
    memcpy(&previous.point, actor + 0x44, sizeof previous.point);
#endif
    func_80015978(position, (u16 *)endpoint);
    if (func_80135004(1, &previous.point, endpoint)) {
        func_80015954((u16 *)endpoint, position);
        if ((s16)D_801152A8.y >= -1400)
            return 0x8000;
        if ((s16)D_801152A8.y >= -3019)
            return 0x4000;
        return 0x2000;
    }
    return 0;
}
