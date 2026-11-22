/* Regional floor intersection and side-plane checks. */
#include "floor_plane_probe.h"

extern u32 D_8017F68C;
extern u32 D_8017F690;
extern u32 D_8017F694;

#if !defined(__mips__) || defined(__LP64__)
#include <string.h>

static s32 signed_word(u32 bits)
{
    if (bits <= 0x7fffffffu) {
        return (s32)bits;
    }
    return -1 - (s32)~bits;
}

static s16 signed_height(s32 value)
{
    s32 low = (s32)((u32)value & 0xffffu);
    return (s16)(low <= 32767 ? low : low - 65536);
}
#endif

s32 func_801347A0(s16 flag, s16 *record, FloorPlane *planes, s32 *table)
{
    FloorPlane *plane = &planes[record[1]];
    s32 offset = table[record[2]];
    s32 distance;
    s32 denominator;
    s32 quotient;
    SceneProbeVector *start;
    SceneProbeVector *hit;
    SceneProbeVector *final_start;
    SceneProbeVector *final_hit;

    if (func_80134A28(plane, FLOOR_ADDRESS(SceneProbeVector, D_8017F690), offset) >= 0) {
        return 0;
    }
    distance = func_80134A28(plane, FLOOR_ADDRESS(SceneProbeVector, D_8017F68C), offset);
    if (distance < 0) {
        return 0;
    }
    distance = -distance;
    denominator = plane->v[1] * 48;
#if !defined(__mips__) || defined(__LP64__)
    if (denominator == 0) {
        bfm_floor_plane_divide_by_zero();
        return 0;
    }
    quotient = signed_word((u32)distance * 48u) / denominator;
#else
    quotient = (s32)((u32)distance * 48u) / denominator;
#endif
    start = FLOOR_ADDRESS(SceneProbeVector, D_8017F68C);
    hit = FLOOR_ADDRESS(SceneProbeVector, D_8017F694);
    hit->x = start->x;
#if defined(__mips__) && !defined(__LP64__)
    hit->y = start->y + quotient;
#else
    hit->y = signed_height(start->y + quotient);
#endif
    hit->z = start->z;
    if (func_80134A28(&planes[record[3]], hit, table[record[4]]) < -0x2F00) {
        return 0;
    }
    if (func_80134A28(&planes[record[5]], FLOOR_ADDRESS(SceneProbeVector, D_8017F694),
                      table[record[6]]) < -0x2F00) {
        return 0;
    }
    if (func_80134A28(&planes[record[7]], FLOOR_ADDRESS(SceneProbeVector, D_8017F694),
                      table[record[8]]) < -0x2F00) {
        return 0;
    }
    if (flag != 0) {
        if (func_80134A28(&planes[record[9]], FLOOR_ADDRESS(SceneProbeVector, D_8017F694),
                          table[record[10]]) < -0x2F00) {
            return 0;
        }
    }
    if ((record[0] & 0x300) != 0) {
        return 0;
    }

    final_start = FLOOR_ADDRESS(SceneProbeVector, D_8017F68C);
    final_hit = FLOOR_ADDRESS(SceneProbeVector, D_8017F694);
    final_hit->x = final_start->x;
    final_hit->z = final_start->z;
#if defined(__mips__) && !defined(__LP64__)
    __builtin_memcpy(&D_801152A8, plane, sizeof(*plane));
#else
    memmove(&D_801152A8, plane, sizeof(*plane));
#endif
    FLOOR_ADDRESS(SceneProbeVector, D_8017F68C)->pad = plane->v[1];
    return 1;
}
