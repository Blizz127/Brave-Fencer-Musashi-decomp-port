/* SPDX-License-Identifier: MIT */
#include "psx_types.h"

/* First-party effect-pool owner layout shared with the other forest actors. */
typedef struct {
    u16 id;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xfc];
} ForestEffectSource;

typedef struct {
    u8 before_height[0x52];
    s16 height;
} ForestNoticeMarker;

extern s32 func_8012C588(s16 effect, ForestEffectSource *source);

/* Attach the notice marker at the actor's requested vertical offset. */
s32 func_80142414(ForestEffectSource *source, s16 height)
{
    s32 address = func_8012C588(19, source);
    if (address) {
        ForestNoticeMarker *marker = (ForestNoticeMarker *)(unsigned long)(u32)address;
        marker->height = height;
    }
    return address;
}
