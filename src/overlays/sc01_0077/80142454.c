/* SPDX-License-Identifier: MIT */
#include "psx_types.h"

typedef struct {
    u16 x, y, z;
    u16 kind, variant, flags;
    u8 reserved[2];
    s16 yaw;
    u32 user;
} NoticePlacement;

typedef struct {
    u8 before_height[0x52];
    s16 height;
    u8 before_anchor[0x10];
    u32 anchor;
} AttachedNotice;

extern u16 D_80126B62, D_80126B66;
extern u8 D_80126B58[];
extern s32 func_8012C51C(void *placement, u8 *owner);

/* Spawn the variant that follows the player at a caller-selected height. */
void func_80142454(s16 height)
{
    NoticePlacement placement;
    AttachedNotice *notice;
    placement.kind = 19;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    /* The original 32-bit player record supplies an aligned halfword. */
    placement.x = *(u16 *)(D_80126B58 + 6);
#else
    placement.x = (u16)(D_80126B58[6] | (D_80126B58[7] << 8));
#endif
    placement.y = D_80126B62;
    placement.z = D_80126B66;
    placement.variant = 1;
    placement.flags = 0;
    placement.yaw = 0;
    placement.user = 0;
    notice = (AttachedNotice *)(unsigned long)(u32)func_8012C51C(&placement, 0);
    if (notice) {
        notice->height = height;
        notice->anchor = (u32)(unsigned long)D_80126B58;
    }
}
