/* Project the segment onto the face. A late rejection keeps the projected point. */
#include "forest_floor.h"

/* External LIBGTE VectorNormalSS service. */
extern void func_80047E98(FloorPoint *input, FloorPoint *output);

s32 func_80134C20(s16 flags, FloorFace *face, FloorPoint *vertices, s32 *distances)
{
    FloorPoint *normal = &vertices[face->e[0].v];
    s32 plane = distances[face->e[0].d];
    s32 near_distance;
    s32 denominator;
    u32 numerator;
    s16 mode = flags;

    if (func_80134FB8(normal, D_801870B0, plane) >= 0) {
        return 0;
    }
    near_distance = func_80134FB8(normal, D_801870AC, plane);
    if (near_distance < 0) {
        return 0;
    }
    denominator = func_80134FB8(normal, D_801870B8, 0);
    numerator = 0u - (u32)near_distance;
    /* Low-word products and coordinate additions wrap before narrowing. */
    D_801870B4->x = D_801870AC->x +
        (u32)((s32)(numerator * (s16)D_801870B8->x) / denominator);
    D_801870B4->y = D_801870AC->y +
        (u32)((s32)(numerator * (s16)D_801870B8->y) / denominator);
    D_801870B4->z = D_801870AC->z +
        (u32)((s32)(numerator * (s16)D_801870B8->z) / denominator);

    if (func_80134FB8(&vertices[face->e[1].v], D_801870B4,
                      distances[face->e[1].d]) < -0x2F00) {
        return 0;
    }
    if (func_80134FB8(&vertices[face->e[2].v], D_801870B4,
                      distances[face->e[2].d]) < -0x2F00) {
        return 0;
    }
    if (func_80134FB8(&vertices[face->e[3].v], D_801870B4,
                      distances[face->e[3].d]) < -0x2F00) {
        return 0;
    }
    if (flags < 0 &&
        func_80134FB8(&vertices[face->e[4].v], D_801870B4,
                     distances[face->e[4].d]) < -0x2F00) {
        return 0;
    }
    if (mode & 1) {
        if (face->flags & 0x300) {
            return 0;
        }
    } else if (face->flags & 0x200) {
        D_801D9500 = face->flags;
        return 0;
    }

    D_801152A8 = *normal;
    func_80047E98(D_801870B8, D_801870B8);
    D_801870B4->x -= ((s16)D_801870B8->x >> 11);
    D_801870B4->y -= ((s16)D_801870B8->y >> 11);
    D_801870B4->z -= ((s16)D_801870B8->z >> 11);
    return 1;
}
