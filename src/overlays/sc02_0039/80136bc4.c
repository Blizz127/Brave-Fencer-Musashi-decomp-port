/* Convert the six offsets in a freshly loaded mine grid to 32-bit addresses. */
#include "psx_types.h"

#if !defined(__mips__) || defined(__LP64__)
#include <stdint.h>
#endif

typedef struct {
    u16 origin_x, origin_z;
    u16 width, height;
    u32 sections[6];
} MineGridHeader;

void func_80136BC4(void *primary) {
    MineGridHeader *grid = primary;

    if (grid->sections[0] == 0x20) {
#if defined(__mips__) && !defined(__LP64__)
        u32 base = (u32)primary;
#else
        u32 base = (u32)(uintptr_t)primary;
#endif
        grid->sections[0] = base + 0x20;
        grid->sections[1] += base;
        grid->sections[2] += base;
        grid->sections[3] += base;
        grid->sections[4] += base;
        grid->sections[5] += base;
    }
}
