/* Clear all four stat displays when the mine initializes its field scene. */
#include "mine_frame_slots.h"

extern void func_80016714(s8 *destination, s32 size);

void func_8016E8F0(void)
{
    func_80016714((s8 *)D_80115218, sizeof(D_80115218));
}
