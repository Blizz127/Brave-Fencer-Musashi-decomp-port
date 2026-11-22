/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_8012A988 over each of the 0x60 records in D_801202A0 when D_801270C0 == 1 or the record's 0x1000 flag is set. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x72];
    u16 flags; /* 0x72 */
    u8 pad1[0x10C - 0x74];
} Rec8012a908; /* 0x10C */

extern Rec8012a908 D_801202A0[];
extern s16 D_801270C0;

extern void func_8012A988(Rec8012a908 *r);

void func_8012A908(void)
{
    s32 i;
    Rec8012a908 *r;

    r = D_801202A0;
    for (i = 0; i < 0x60; i++) {
        if (D_801270C0 == 1 || (r->flags & 0x1000)) {
            func_8012A988(r);
        }
        r++;
    }
}
