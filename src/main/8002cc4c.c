/* Clears the byte at offset 0xA of every 0xC-byte entry in four tables. */
#include "psx_types.h"

typedef struct Entry12 {
    u8 pad[0xA];
    u8 flag;
    u8 pad2;
} Entry12;

extern u32 D_8006A970[4];
extern s32 D_80065504[4];

void func_8002CC4C(void) {
    s32 i;
    s32 j;
    Entry12 *e;
    u32 *p;

    p = D_8006A970;
    for (i = 0; i < 4; i++, p++) {
        for (j = 0, e = (Entry12 *)*p; j < D_80065504[i]; e++) {
            j++;
            e->flag = 0;
        }
    }
}
