/* Owner re-derivation from retail assembly and owner types. */
#include "psx_types.h"
#ifdef __mips__
void
#else
u32
#endif
func_8001C9D0(u8 *p) {
    *(u16 *)(p+0x1C)=4096; *(u16 *)(p+0x1A)=4096; *(u16 *)(p+0x18)=4096;
    *(u16 *)(p+0x14)=0; *(u16 *)(p+0x12)=0; *(u16 *)(p+0x10)=0;
    *(u16 *)(p+0xC)=0; *(u16 *)(p+0xA)=0; *(u16 *)(p+8)=0;
    *(u32 *)(p+4)=0; *(u32 *)(p+0x20)=0;
    *(u16 *)(p+0x28)=200; *(u16 *)(p+0x2A)=200;
    *(u16 *)(p+0x2C)=0; *(u16 *)(p+0x2E)=0; *(u32 *)(p+0x80)=0;
#ifndef __mips__
    return 200;
#endif
}
