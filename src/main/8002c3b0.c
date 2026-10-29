/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the total number of 8 KB blocks needed by the first `count` file entries (sizes rounded up). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    s32 size;
    u8 pad1C[0x28 - 0x1C];
} FileEntry;

s32 func_8002C3B0(s32 count, FileEntry *entry) {
    s32 unused_frame[1]; /* retail reserves an unused stack slot */
    s32 i;
    s32 blocks;
    s32 q;

    blocks = 0;
    for (i = 0; i < count; i++, entry++) {
        q = entry->size / 0x2000;
        if (entry->size & 0x1FFF) {
            blocks += q + 1;
        } else {
            blocks += q;
        }
    }
    return blocks;
}
