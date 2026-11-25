#include "room_descriptor.h"

extern s16 D_800B9A08;
extern s32 func_800CFC5C(s32 id);
extern s32 func_800CFD68(s32 id);
extern s32 func_800168C4(s32 index);
extern s32 func_800167F0(u16 index);

s32 func_800D1EBC(void)
{
    s32 mode;
    s32 group;
    u16 result;

    if (D_800B9A08 != 0x3005) {
        group = func_800CFC5C(D_800B9A08);
        mode = 2;
        if (group) {
            goto select;
        }
        if (func_800CFD68(ROOM_DESCRIPTOR_ID)) {
            group = func_800CFC5C(ROOM_DESCRIPTOR_ID);
            mode = 1;
            if (!group) {
                goto select;
            }
        }
    }
    mode = 2;
select:
    if (mode < 0) {
        goto alternate;
    }
    if (mode >= 2) {
        goto alternate;
    }
    result = func_800168C4(4);
    goto done;
alternate:
    result = func_800167F0(4);
done:
    return result != 0;
}
