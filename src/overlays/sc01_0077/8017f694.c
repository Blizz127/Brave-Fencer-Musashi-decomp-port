#include "psx_types.h"

/* Compatible with the shared clean effect allocator's actor type. */
typedef struct {
    u16 id;
    u8 p2[4];
    u16 f6;
    u8 p8[2];
    u16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0xfc];
} ForestEffectSource;

extern s32 func_8012C588(s16 effect, ForestEffectSource *source);

void func_8017F694(u8 *actor)
{
    u8 *model = (u8 *)(unsigned long)*(u32 *)(actor + 0x20);
    *(u16 *)(actor + 2) = 3;
    *(u16 *)(model + 0x10) = 0x400;
    *(u16 *)(actor + 0x5c) |= 0xcc00;
    func_8012C588(0x25, 0);
}
