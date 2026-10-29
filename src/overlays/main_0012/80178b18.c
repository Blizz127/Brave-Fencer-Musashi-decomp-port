/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x0C];
    s32 f0C;
} Rec80181CB0;

extern Rec80181CB0 D_80181CB0;
extern void func_80178BF8(void);
extern void func_801746A4(void);
extern void func_8017435C(void);
extern void func_80178CBC(void *a, void *b);

s32 func_80178B18(void *x, s32 y) {
    D_80181CB0.f0C = y;
    func_80178BF8();
    func_801746A4();
    func_8017435C();
    func_80178CBC(x, &D_80181CB0);
}
