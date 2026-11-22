/* Reserve the first unused entry in the eight-slot joint-effect list. */
#include "psx_types.h"

extern u8 D_801E44F0[8];

s32 func_8018B300(void)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (!D_801E44F0[i]) {
            D_801E44F0[i] = 1;
            return i;
        }
    }
    return -1;
}
