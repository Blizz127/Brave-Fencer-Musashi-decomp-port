#include "psx_types.h"

extern u8 D_800AF630[];

/* The original compiler emits aligned halfword accesses. On the host this
 * symbol denotes byte-addressed, little-endian guest storage. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define SCENE_ADVANCE(field) (++*(u16 *)(field))
#define SCENE_CLEAR(field) (*(u16 *)(field) = 0)
void func_800118AC(void)
{
    register u8 *state = D_800AF630;
    SCENE_ADVANCE(state + 0xA3B4);
    SCENE_CLEAR(state + 0xA3BA);
    SCENE_CLEAR(state + 0xA3BE);
    SCENE_CLEAR(state + 0xA3B6);
    SCENE_CLEAR(state + 0xA3BC);
}
#else
#include "musashi_scene_counter.h"

void func_800118AC(void)
{
    (void)musashi_advance_scene_fields(D_800AF630 + 0xA3B4);
}
#endif
