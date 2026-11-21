/* Own retail: modulo-32 position updates, then fresh movement samples. */
#include "psx_types.h"
void func_801473EC(u8 *obj)
{
    #if defined(__GNUC__) && __GNUC__ == 2 && defined(__mips__)
    register u32 x __asm__("$2");
    register u32 y __asm__("$3");
    register u32 z __asm__("$2");
    register u32 vx __asm__("$4");
    register u32 vy __asm__("$5");
    register u32 vz __asm__("$3");
#else
    u32 x, y, z, vx, vy, vz;
#endif
    x = *(u32 *)(obj + 4);
    vx = *(u32 *)(obj + 0x10);
    y = *(u32 *)(obj + 8);
    vy = *(u32 *)(obj + 0x14);
    x += vx;
    *(u32 *)(obj + 4) = x;
    z = *(u32 *)(obj + 0xC);
    y += vy;
    *(u32 *)(obj + 8) = y;
    vz = *(u32 *)(obj + 0x18);
    vx = *(u32 *)(obj + 0x10);
    z += vz;
    *(u32 *)(obj + 0xC) = z;
    if (vx || *(u32 *)(obj + 0x14) || *(u32 *)(obj + 0x18))
        *(u32 *)(obj + 0x44) |= 0x80000000u;
}
