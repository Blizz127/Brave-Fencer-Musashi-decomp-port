/* Owner re-derivation from retail assembly (clean input set). */
/* Resumable LZ decompressor step: consume up to 0x800 source bytes per call via a 0x400 scratchpad window. */
#include "psx_types.h"

extern u8 D_800747A0;
extern u8 D_800747A4;
extern u32 D_800747AC;
extern u32 D_800747B0;
extern u16 D_800747B4;
extern s32 D_800C7D24;

s32 func_80018730(u8 *src)
{
    s32 left;
    u32 pos;
    u8 mask;
    u8 flags;
    u8 *dst;
    u32 state;
    u16 code;
    u8 *ring;
    u32 from;
    s32 len;
    u8 c;
    u8 b;
    s32 ret;

    left = 0x800;
    pos = D_800747B0;
    mask = D_800747A0;
    flags = D_800747A4;
    dst = (u8 *)D_800747AC;
    state = D_800C7D24;
    code = D_800747B4;
    ring = (u8 *)0x1F800000;

    switch (state) {
    case 0:
        return 0;
    case 1:
        pos = 1;
        mask = 1;
        flags = *src++;
        left--;
    case 2:
    loop:
        if (flags & mask) {
            c = *src++;
            ring[pos] = c;
            pos = (pos + 1) & 0x3FF;
            left--;
            *dst++ = c;
        } else {
            code = *src++ | (code & 0xFF00);
            left--;
            if (left == 0) {
                ret = 1;
                state = 3;
                goto save;
            }
    case 3:
            code = (code & 0xFF) | (*src++ << 8);
            left--;
            from = code & 0x3FF;
            if (from == 0) {
                D_800C7D24 = 0;
                return 0;
            }
            for (len = (code >> 10) + 2; len != 0; len--) {
                b = ring[from];
                from = (from + 1) & 0x3FF;
                ring[pos] = b;
                pos = (pos + 1) & 0x3FF;
                *dst++ = b;
            }
        }
        if (left == 0) {
            ret = 1;
            state = 4;
        save:
            D_800C7D24 = state;
            D_800747B0 = pos;
            D_800747A0 = mask;
            D_800747A4 = flags;
            D_800747AC = (u32)dst;
            D_800747B4 = code;
            return ret;
        }
    case 4:
        if (mask == 0x80) {
            mask = 1;
            flags = *src++;
            left--;
            if (left == 0) {
                D_800C7D24 = 2;
                D_800747B0 = pos;
                D_800747A0 = mask;
                D_800747A4 = flags;
                D_800747AC = (u32)dst;
                D_800747B4 = code;
                return 1;
            }
        } else {
            mask <<= 1;
        }
        goto loop;
    }
#ifndef __mips__
    /* Host (native-lane) builds only: retail falls off the switch for state >= 5
     * and returns 0 through the stale sltiu result; make that explicit here. The
     * MIPS matching build never sees this line. */
    return 0;
#endif
}
