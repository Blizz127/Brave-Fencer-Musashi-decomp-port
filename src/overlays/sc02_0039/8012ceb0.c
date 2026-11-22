#include "mine_terrain_vector.h"

s32 func_8012CEB0(MineActionPoint *from, MineActionPoint *to, s32 mode)
{
#if defined(__mips__) && !defined(__LP64__)
    MineActionPoint vertical;
#else
    /* The mode-two query replaces the temporary endpoint before choosing a surface. */
    MineActionPoint vertical = {0};
#endif
    s32 first, second;
    u8 action;

    if (mode & 1) {
        first = func_80133784(mode, from, to);
        action = to->surface.action;
        first |= action;
        if (action == 2 || action == 27)
            first |= 0x1000;
        return first;
    }
    first = func_80133784(0, from, to);
    if (first == 0)
        return 0;
    second = func_80133784(2, to, &vertical);
    if (!(second & 0x6000))
        return 0;
    second &= ~0x8000;
    second |= first & 0x8000;
    to->y = vertical.y;
    action = to->surface.action;
    second |= action;
    if (action == 2 || action == 27)
        second |= 0x1000;
    return second;
}
