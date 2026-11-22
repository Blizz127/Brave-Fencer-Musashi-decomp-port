#include "tower_postfire_camera.h"
#include "forest_player_state.h"

#if defined(__mips__) && !defined(__LP64__)
#define WRITE_WORD(p, v) (*(u32 *)(p) = (v))
#define WRITE_HALF(p, v) (*(u16 *)(p) = (v))
#else
static void write_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void write_word(u8 *p, u32 value)
{
    write_half(p, (u16)value);
    write_half(p + 2, (u16)(value >> 16));
}
#define WRITE_WORD(p, v) write_word((p), (v))
#define WRITE_HALF(p, v) write_half((p), (v))
#endif

s32 func_8012A048(u8 *camera, u32 callback_address, u8 mode)
{
    u16 x, y, z;
    WRITE_WORD(camera, callback_address);
    camera[4] = mode;
    WRITE_WORD(camera + 0x9c, 0);
    WRITE_HALF(camera + 0xa0, 0);
    WRITE_HALF(camera + 0xa2, 0);
    x = D_80126B58.position[0] >> 16;
    y = D_80126B58.position[1] >> 16;
    z = D_80126B58.position[2] >> 16;
    D_80126940 = x;
    D_80126942 = (s16)y;
    D_80126944 = (s16)z;
    return x;
}
