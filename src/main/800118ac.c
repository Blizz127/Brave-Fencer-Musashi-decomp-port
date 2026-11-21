#include "psx_types.h"

extern u8 D_800AF630[];

/* The original compiler emits aligned halfword accesses. On the host this
 * symbol denotes byte-addressed, little-endian guest storage. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define SCENE_ADVANCE(field) (++*(u16 *)(field))
#define SCENE_CLEAR(field) (*(u16 *)(field) = 0)
#else
static void scene_advance(u8 *field)
{
    u16 value = (u16)(field[0] | ((u16)field[1] << 8));
    value = (u16)(value + 1u);
    field[0] = (u8)value;
    field[1] = (u8)(value >> 8);
}

static void scene_clear(u8 *field)
{
    field[0] = 0;
    field[1] = 0;
}
#define SCENE_ADVANCE(field) scene_advance(field)
#define SCENE_CLEAR(field) scene_clear(field)
#endif

void func_800118AC(void)
{
    register u8 *state = D_800AF630;
    SCENE_ADVANCE(state + 0xA3B4);
    SCENE_CLEAR(state + 0xA3BA);
    SCENE_CLEAR(state + 0xA3BE);
    SCENE_CLEAR(state + 0xA3B6);
    SCENE_CLEAR(state + 0xA3BC);
}
