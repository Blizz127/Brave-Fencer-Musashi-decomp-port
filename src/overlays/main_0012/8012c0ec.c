/* Owner re-derivation from retail assembly (clean input set). */
/* Runs the optional per-object check hook (or a distance test against D_80126CAC) and, if it passes, clears the high flag of the linked record at +0x68 and calls func_8012C218; returns whether it passed. */
#include "psx_types.h"
extern int (*D_801274D4)(char *);
extern char D_80126CAC[];
extern int D_801274E0;
int func_80013478(char *a, char *b);
void func_8012C218(char *o);
int func_8012C0EC(char *o) {
    int r;
    if (D_801274D4 == 0) {
        r = (func_80013478(o + 4, D_80126CAC) < D_801274E0) ^ 1;
    } else {
        r = D_801274D4(o);
    }
    if (r == 0) {
        return 0;
    }
    if (*(int *)(o + 0x68) != 0 && (*(short *)(o + 0x72) & 0x8000) != 0) {
        *(u16 *)(*(int *)(o + 0x68) + 0xA) &= 0x7FFF;
    }
    func_8012C218(o);
    return 1;
}
