/* Intersect the probe with the floor plane and reject points outside the remaining face planes. */
#include "forest_floor.h"

s32 func_801347A0(s16 five_planes, FloorFace *face, FloorPoint *vertices, s32 *distances)
{
    FloorPoint *normal;
    s32 plane_bias;
    s32 near_distance;
    s32 height_delta;
    s32 denominator;

    normal = &vertices[face->e[0].v];
    plane_bias = distances[face->e[0].d];
    if (func_80134A28(normal, D_801870B0, plane_bias) >= 0) {
        return 0;
    }
    near_distance = func_80134A28(normal, D_801870AC, plane_bias);
    if (near_distance < 0) {
        return 0;
    }
    denominator = (s16)normal->y * 0x30;
    height_delta = (s32)((0u - (u32)near_distance) * 0x30u) / denominator;
    D_801870B4->x = D_801870AC->x;
    D_801870B4->y = D_801870AC->y + height_delta;
    D_801870B4->z = D_801870AC->z;
    if (func_80134A28(&vertices[face->e[1].v], D_801870B4, distances[face->e[1].d]) < -0x2F00) {
        return 0;
    }
    if (func_80134A28(&vertices[face->e[2].v], D_801870B4, distances[face->e[2].d]) < -0x2F00) {
        return 0;
    }
    if (func_80134A28(&vertices[face->e[3].v], D_801870B4, distances[face->e[3].d]) < -0x2F00) {
        return 0;
    }
    if (five_planes != 0) {
        if (func_80134A28(&vertices[face->e[4].v], D_801870B4, distances[face->e[4].d]) < -0x2F00) {
            return 0;
        }
    }
    if (face->flags & 0x300) {
        return 0;
    }
    D_801870B4->x = D_801870AC->x;
    D_801870B4->z = D_801870AC->z;
    D_801152A8 = *normal;
    D_801870AC->w = normal->y;
    return 1;
}
