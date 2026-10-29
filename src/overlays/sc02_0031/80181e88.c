/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a 0x1C-byte packet from two unaligned words of a0, a u16 constant and three u32
 * constants, then calls func_80016A3C(&packet). */
#include "psx_types.h"

typedef struct {
    u32 w;
} __attribute__((packed)) PW80181E88;

extern void func_80016A3C(void *p);

void func_80181E88(u8 *a0)
{
    u8 buf[0x1C];

    *(PW80181E88 *)(buf + 0x00) = *(PW80181E88 *)(a0 + 0x00);
    *(PW80181E88 *)(buf + 0x08) = *(PW80181E88 *)(a0 + 0x04);
    *(u16 *)(buf + 0x04) = 1;
    *(u32 *)(buf + 0x10) = 0x00808080;
    *(u32 *)(buf + 0x14) = 0;
    *(u32 *)(buf + 0x18) = 0x50000000;
    func_80016A3C(buf);
}
