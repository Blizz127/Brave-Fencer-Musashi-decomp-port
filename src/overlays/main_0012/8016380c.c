/* Sweep from the actor's saved position and apply a clipped endpoint. */
#include "forest_floor.h"
#ifndef __mips__
#include <string.h>
#endif

extern void func_80015978(u16 *source, u16 *destination);
extern void func_80015954(u16 *source, u16 *destination);

typedef struct { u8 bytes[8]; } SavedPointBytes;

s32 func_8016380C(u8 *actor, FloorPoint *endpoint)
{
    union {
        SavedPointBytes raw;
        FloorPoint point;
    } previous;
    s32 result;
    u16 *position = (u16 *)(actor + 4);

#ifdef __mips__
    /* The original copies the eight bytes without assuming word alignment. */
    previous.raw = *(const SavedPointBytes *)(actor + 0x44);
#else
    memcpy(&previous.point, actor + 0x44, sizeof previous.point);
#endif
    func_80015978(position, (u16 *)endpoint);
    result = func_80133784(1, &previous.point, endpoint);
    if (result != 0) {
        func_80015954((u16 *)endpoint, position);
        return result;
    }
    return 0;
}
