/* Owner re-derivation from retail assembly (clean input set). */
/* Copies six signed bytes into two triples of halfwords. */
#include "psx_types.h"

void func_801485B8(s8 *src, s16 *dst0, s16 *dst1) {
    dst0[0] = src[0];
    dst0[1] = src[1];
    dst0[2] = src[2];
    dst1[0] = src[3];
    dst1[1] = src[4];
    dst1[2] = src[5];
}
