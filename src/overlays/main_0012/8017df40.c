/* Owner re-derivation from retail assembly (clean input set). */
/* Copies the saved menu state between the working buffers and their backups: with dir == 0 the backups D_8018581C (0x98 bytes), D_801858B4 (0x40 halfwords), D_80185934 (0x40 bytes), D_80185974 (0x100 bytes) and D_80185A74 (0x18 words) are copied into D_80078E78, D_800A6588, D_800AE648, D_800BA1B8 and D_800BA2B8; for any other dir the same copies run the other way. */
#include "psx_types.h"

typedef struct {
    u32 w[0x26];
} Blk98;

typedef struct {
    u8 b[4];
} Raw4;

extern Blk98 D_80078E78;
extern Blk98 D_8018581C;
extern u16 D_800A6588[];
extern u16 D_801858B4[];
extern u8 D_800AE648[];
extern u8 D_80185934[];
extern u8 D_800BA1B8[];
extern u8 D_80185974[];
extern Raw4 D_800BA2B8[];
extern Raw4 D_80185A74[];

void func_8017DF40(s16 dir)
{
    s16 i;

    if (dir == 0) {
        D_80078E78 = D_8018581C;
        for (i = 0; i < 0x40; i++) {
            D_800A6588[i] = D_801858B4[i];
        }
        for (i = 0; i < 0x40; i++) {
            D_800AE648[i] = D_80185934[i];
        }
        for (i = 0; i < 0x100; i++) {
            D_800BA1B8[i] = D_80185974[i];
        }
        for (i = 0; i < 0x18; i++) {
            D_800BA2B8[i] = D_80185A74[i];
        }
    } else {
        D_8018581C = D_80078E78;
        for (i = 0; i < 0x40; i++) {
            D_801858B4[i] = D_800A6588[i];
        }
        for (i = 0; i < 0x40; i++) {
            D_80185934[i] = D_800AE648[i];
        }
        for (i = 0; i < 0x100; i++) {
            D_80185974[i] = D_800BA1B8[i];
        }
        for (i = 0; i < 0x18; i++) {
            D_80185A74[i] = D_800BA2B8[i];
        }
    }
}
