/* func_80150170 [80150170,801502EC) - owner re-derivation from retail assembly (clean input set). */
/* Builds two probe points from the actor's position (second one 0x10 higher), queries both, and stores the nearer hit (or the default marker) into the +0x78 record, updating the +0x7E flag. */
#include "floor_providers.h"


s32 func_80150170(u8 *obj) {
    SceneProbeVector a;
    SceneProbeVector b;
    SceneProbeVector c;
    s32 r1;
    s32 r2;

    b.x = *(u16 *)(obj + 6);
    a.x = b.x;
    b.y = *(u16 *)(obj + 0xA) - 0x10;
    a.y = b.y;
    b.z = *(u16 *)(obj + 0xE);
    a.z = b.z;
    r1 = func_801502EC(obj, &a, &c);
    r2 = func_80150460(obj, &b);
    if (r2 == 0) {
        b.y = 0x7FFF;
    }
    if ((r1 | r2) != 0) {
        if (a.y < b.y) {
            *(u16 *)(obj + 0x78) = a.x;
            *(u16 *)(obj + 0x7A) = a.y;
            *(u16 *)(obj + 0x7C) = a.z;
            *(SceneProbeVector *)(obj + 0x80) = c;
        } else {
            *(u16 *)(obj + 0x78) = b.x;
            *(u16 *)(obj + 0x7A) = b.y;
            *(u16 *)(obj + 0x7C) = b.z;
            *(SceneProbeVector *)(obj + 0x80) = D_801152A8;
        }
        *(u16 *)(obj + 0x7E) &= 0xFFFE;
        return 1;
    }
    *(u16 *)(obj + 0x78) = a.x;
    *(u16 *)(obj + 0x7A) = 0x7FFF;
    *(u16 *)(obj + 0x7C) = a.z;
    *(u16 *)(obj + 0x7E) |= 1;
    return 0;
}
