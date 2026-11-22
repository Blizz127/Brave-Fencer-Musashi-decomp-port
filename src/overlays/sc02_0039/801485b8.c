/* Expand the packed pose coordinates in order, preserving source and destination overlap. */
#include "mine_pose_points.h"

#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
static void store_component(u8 *output, u8 sample)
{
    output[0] = sample;
    output[1] = (sample & 0x80) ? 0xff : 0;
}
#endif

void func_801485B8(const u8 *source, u8 *first, u8 *second)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    /* The historical compiler schedules reads through this original signed-byte view. */
    s8 *bytes = (s8 *)source;
    s16 *a = (s16 *)first;
    s16 *b = (s16 *)second;

    a[0] = bytes[0];
    a[1] = bytes[1];
    a[2] = bytes[2];
    b[0] = bytes[3];
    b[1] = bytes[4];
    b[2] = bytes[5];
#else
    store_component(first, source[0]);
    store_component(first + 2, source[1]);
    store_component(first + 4, source[2]);
    store_component(second, source[3]);
    store_component(second + 2, source[4]);
    store_component(second + 4, source[5]);
#endif
}
