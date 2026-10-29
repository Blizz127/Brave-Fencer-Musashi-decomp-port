/* Owner re-derivation from retail assembly and locally generated m2c. */
#include "psx_types.h"

typedef struct {
    s32 field0;
    s32 field4;
    s16 field8;
    s16 fieldA;
} BfmWork;

extern BfmWork D_800A4ECC;
extern void func_8003B45C(BfmWork *);

void func_8002D1F0(s16 arg0) {
    D_800A4ECC.field0 = 1;
    D_800A4ECC.field4 = arg0;
    D_800A4ECC.field8 = 0;
    D_800A4ECC.fieldA = 0;
    func_8003B45C(&D_800A4ECC);
}
