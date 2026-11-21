/* Refresh the five variable wallet digits between the coin and the fixed zero. */
#include "psx_types.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
typedef struct {
    u8 prefix[10];
    u16 y;
    u8 u;
    u8 suffix[7];
} WalletDigit;

void func_80177F84(u8 *packet, s32 packed)
{
    WalletDigit *sprite = (WalletDigit *)packet;
    s32 hide = 0x100;
    s16 index = 0;
    do {
        s16 digit = (packed >> 16) & 15;
        s32 value = digit;
        s32 y;
        packed = (s32)((u32)packed << 4);
        if (index == 5 || value != 0)
            hide = 0;
        ++sprite;
        ++index;
        y = sprite->y;
        sprite->y = (u16)(hide | (y & ~0x100));
        sprite->u = (u8)(8 + digit * 8);
    } while (index < 5);
}
#else
void func_80177F84(u8 *packet, s32 packed)
{
    u32 stream = (u32)packed;
    u8 hide = 1;
    unsigned i;
    for (i = 0; i < 5; ++i) {
        u8 digit = (u8)((stream >> 16) & 15u);
        stream <<= 4;
        if (digit != 0)
            hide = 0;
        packet += 20;
        /* A hidden sprite sits 256 lines lower; Y is a little-endian halfword. */
        packet[11] = (u8)((packet[11] & ~1u) | hide);
        packet[12] = (u8)(8 + digit * 8);
    }
}
#endif
