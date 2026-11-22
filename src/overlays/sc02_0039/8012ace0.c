/* Return the first animation index that stops this sequence. */
#include "mine_animation_frames.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u16 unused[2];
    s16 operation;
    u16 argument;
} MineAnimationFrame;
#endif

s32 func_8012ACE0(Obj8014A2E4 *actor)
{
    s32 index = 0;
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    MineAnimationFrame *frames = *(MineAnimationFrame **)((u8 *)actor + 0x90);
#else
    const u8 *key;
    u32 address;
    if (!actor)
        return -1;
    key = (const u8 *)actor + 0x90;
    address = (u32)key[0] | (u32)key[1] << 8 | (u32)key[2] << 16 | (u32)key[3] << 24;
    if (!address || (address & 1u) || address > 0xfffffffau)
        return -1;
    address += 4;
#endif
    for (;; index++) {
#if BFM_MINE_DIRECT_GUEST_ADDRESS
        switch (frames[index].operation) {
#else
        s16 operation;
        if (!bfm_animation_frame_operation(address, &operation))
            return -1;
        switch (operation) {
#endif
        case -50:
        case -5:
        case -4:
        case -3:
            break;
        case -2:
        case -1:
        case 0:
            return index;
        }
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        if (address > 0xfffffff6u)
            return -1;
        address += 8;
#endif
    }
}
