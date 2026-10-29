/* Owner re-derivation from retail assembly (clean input set). */
/* If func_80128ED8 accepts the position it runs func_80146C3C, otherwise it runs func_800D22E4 and damps two velocity words to 3/4. */
#include "psx_types.h"

extern s32 func_80128ED8(void *a, void *b);
extern void func_80146C3C(void *obj);
extern void func_800D22E4(void *obj);

void func_801661CC(u8 *obj) {
    s32 a;
    s32 b;
    if (func_80128ED8(*(void **)(obj + 0x20), obj + 0x24) != 0) {
        func_80146C3C(obj);
    } else {
        func_800D22E4(obj);
        a = *(s32 *)(obj + 0x10);
        b = *(s32 *)(obj + 0x18);
        *(s32 *)(obj + 0x10) = (a >> 1) + (a >> 2);
        *(s32 *)(obj + 0x18) = (b >> 1) + (b >> 2);
    }
}
