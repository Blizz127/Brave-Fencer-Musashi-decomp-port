/* Owner re-derivation from retail assembly (clean input set). */
/* Streams the next 32 KB of the current audio file when the CD is ready, advancing to the next file on end (-1) and retrying on -2. */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    s16 file;
    u8 pad4[0x10 - 4];
    u8 done;
    u8 pad11[0x18 - 0x11];
} StreamFile;

extern s16 func_80042154(s32, s32, s16);
extern s16 func_80042374(s32);
extern StreamFile D_800A4640[];
extern s32 D_800A46C0;
extern s32 D_800A46C4;
extern u16 D_800A46CC;
extern s16 D_800A46CE;

s32 func_80030634(void) {
    s16 result;
    s32 index;
    u16 *count;

    if (func_80042374(0) == 0) {
        return 0;
    }
    index = D_800A46CE;
    result = func_80042154(D_800A46C0 + D_800A46C4, 0x8000, D_800A4640[index].file);
    switch (result) {
    case -2:
        D_800A46C4 += 0x8000;
        break;
    case -1:
        D_800A4640[index].done = 1;
        D_800A46CC++;
        break;
    default:
        count = &D_800A46CC;
        (*count)++;
        break;
    }
    return 0;
}
