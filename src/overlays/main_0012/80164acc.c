/* func_80164ACC [80164ACC,80164BA0) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

/* pool entry: only the u16 at +0x60 is touched here. */
typedef struct {
    u8 p0[0x60];
    s16 f60;
    u8 p62[0xAA];
} Ent;

/* callers read v0: no (the direct caller ignores it; retail sets none); void.
 * func_8014C308/8014C278/8014C3A4 are called with their owner prototypes (pointers cast to s32 where the
 * owner prototype takes s32). */
extern u8 D_801202A0[];
extern s32 func_8014C308(void *o, void *e, s32 k, u16 mask);
extern int func_8014C278(char *a, char *b, int limit);
extern void func_80015978(u8 *src, u16 *dst);
extern void func_8014C3A4(void *o, s32 b, s32 c, s32 d);

void func_80164ACC(u8 *o) {
    u16 v[4];
    Ent *e;

    for (e = ((Ent *)&D_801202A0); e < &((Ent *)&D_801202A0)[96]; e++) {
        if (func_8014C308(o, e, 0xB, 2) != 0) {
            if (func_8014C278((char *)o, (char *)e, 0x50) != 0) {
                func_80015978((u8 *)e + 4, v);
                v[3] = 0;
                func_8014C3A4(o, (s32)e, 0xB, (s32)v);
                e->f60 = e->f60 / 2;
            }
        }
    }
}
