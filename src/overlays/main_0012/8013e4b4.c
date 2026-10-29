/* Owner re-derivation from retail assembly (clean input set). */
/* Walks two zero-terminated 0x14-byte tables: runs func_8012C750 on each entry of the first and clears bit 15 of each entry of the second. */
#include "psx_types.h"
typedef struct { u8 p0[6]; u16 f6; u8 p8[0xC]; } E;
extern E *D_801274C8;
extern u32 D_801274CC;
extern void func_8012C750(E *e);
void func_8013E4B4(void) {
    E *p = D_801274C8;
    E *a;
    if (p && p->f6) {
        do {
            func_8012C750(p);
            p++;
        } while (p->f6);
    }
    p = (E *)D_801274CC;
    if (p && p->f6) {
        a = p;
        do {
            ((u16 *)a)[5] &= 0x7FFF;
            a++;
        } while (a->f6);
    }
}
