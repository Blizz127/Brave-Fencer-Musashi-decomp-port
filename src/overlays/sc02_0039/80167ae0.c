#include "mine_target_draw.h"

void func_80167AE0(u8 *object, s16 *from, s16 *to)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    u32 matrix[8];
    TargetPrimitive p;
#else
    /* Game callbacks may copy the complete records, including their padding. */
    u32 matrix[8] = {0};
    TargetPrimitive p = {0};
#endif
    u8 *point;
    s16 i;

    func_80168070(object, from, to, matrix);
    p.vertex[1].x = p.vertex[1].y = p.vertex[1].z = p.vertex[0].z = p.vertex[2].z = p.vertex[3].z = 0;
    if (D_800B99DA & 1)
        p.color[1].red = 160;
    else if (D_800B99DA & 2)
        p.color[1].red = 192;
    else
        p.color[1].red = 128;
    p.color[1].green = p.color[1].blue = p.color[1].red >> 2;
    p.color[0].red = p.color[0].green = p.color[0].blue = 0;
    p.color[2].red = p.color[2].green = p.color[2].blue = 0;
    p.color[3].red = p.color[3].green = p.color[3].blue = 0;
    p.tag = 0x50000000;
    point = D_801821E4;
    i = 0;
    do {
        p.vertex[0].x = (s8)*point++;
        p.vertex[0].y = (s8)*point++;
        p.vertex[2].x = (s8)*point++;
        p.vertex[2].y = (s8)*point++;
        p.vertex[3].x = (s8)*point++;
        p.vertex[3].y = (s8)*point--;
        func_80017758(&p, matrix);
    } while (++i < 4);

    point = D_801821F8;
    p.vertex[1].x = p.vertex[1].y = p.vertex[0].y = p.vertex[2].y = p.vertex[3].y = 0;
    p.vertex[1].z = -32;
    i = 0;
    do {
        p.vertex[0].x = (s8)*point++;
        p.vertex[0].z = (s8)*point++;
        p.vertex[2].x = (s8)*point++;
        p.vertex[2].z = (s8)*point++;
        p.vertex[3].x = (s8)*point++;
        p.vertex[3].z = (s8)*point--;
        func_80017758(&p, matrix);
        if (i == 1) {
            p.vertex[1].z = 32;
            point += 2;
        }
    } while (++i < 4);

    point += 2;
    p.vertex[0].x = 0;
    p.vertex[0].z = -32;
    p.color[0].red = p.color[1].red;
    p.color[0].green = p.color[0].blue = p.color[1].green;
    i = 0;
    do {
        p.vertex[2].x = (s8)*point++;
        p.vertex[2].z = (s8)*point++;
        p.vertex[3].x = (s8)*point++;
        p.vertex[3].z = (s8)*point++;
        func_80017758(&p, matrix);
    } while (++i < 2);
}
