/* Follow the actor that owns this attachment. One animation sources its
 * position from a joint instead of the owner's actor position. */
#include "psx_types.h"

typedef struct { u8 bytes[8]; } AxisVector;
extern u8 D_80197514[], D_8018AFB0[];
extern void func_8012C218(u8 *actor);
extern void func_8012F214(u8 *owner, u8 *joint, s16 *point);

#define FROM_LINK(address) ((u8 *)(unsigned long)*(u32 *)(address))
#define MODEL(actor) FROM_LINK((actor) + 0x20)
#define OWNER(actor) FROM_LINK((actor) + 0x64)

void func_80184D50(u8 *child)
{
    u8 *owner = OWNER(child);
    s16 point[4];

    if (*(s16 *)(child + 0xfe) == 0x7fff)
        *(s16 *)(child + 0xfe) = *(u16 *)(MODEL(child) + 0x12);
    if (*(s16 *)(child + 0xfc) != *(s16 *)(OWNER(child) + 0x36)) {
        func_8012C218(child);
        return;
    }
    *(AxisVector *)(MODEL(child) + 0x10) = *(AxisVector *)(MODEL(owner) + 0x10);
    if (*(u32 *)(OWNER(child) + 0x90) != (u32)(unsigned long)D_80197514) {
        *(u16 *)(MODEL(child) + 0x12) = *(u16 *)(child + 0xfe);
        *(s32 *)(child + 4) = *(s32 *)(OWNER(child) + 4);
        *(s32 *)(child + 8) = *(s32 *)(OWNER(child) + 8);
        *(s32 *)(child + 12) = *(s32 *)(OWNER(child) + 12);
    } else {
        func_8012F214(OWNER(child), D_8018AFB0, point);
        *(s16 *)(child + 6) = point[0];
        *(s16 *)(child + 10) = point[1];
        *(s16 *)(child + 14) = point[2];
    }
    *(AxisVector *)(MODEL(child) + 0x18) = *(AxisVector *)(MODEL(owner) + 0x18);
    *(u16 *)(MODEL(child) + 0x2c) = *(u16 *)(MODEL(owner) + 0x2c);
    *(u32 *)(MODEL(child) + 4) = *(u32 *)(MODEL(owner) + 4);
}
