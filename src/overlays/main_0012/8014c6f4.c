/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame input/state update: latches the button-held byte at +0x1C3 from the +0x44/+0x48 pad words, copies 8 bytes from the linked record, then records the trail position (+0x154..) and queues func_80015978 for the matching table entry or the object itself. */
#include "psx_types.h"
struct Raw8 {
    char b[8];
};
extern char D_80078E78[];
extern char D_8011F9D0[];
int func_8016F1AC(void);
void func_80015978(char *a, char *b);
void func_8014C6F4(char *o) {
    char *g = D_80078E78;
    char *e;
    char *p;
    int id;
    if ((*(int *)(o + 0x44) & 2) != 0) {
        *(u8 *)(o + 0x1C3) = 1;
    }
    if ((((*(int *)(o + 0x44) ^ *(int *)(o + 0x48)) & *(int *)(o + 0x44)) & 0x200) != 0) {
        *(u8 *)(o + 0x1C3) = 0;
    }
    *(struct Raw8 *)(o + 0x164) = *(struct Raw8 *)(*(int *)(o + 0x20) + 0x10);
    if (func_8016F1AC() != 0 || ((*(int *)(o + 0x44) & 0x200) != 0 && *(u8 *)(o + 0x1C3) == 0)) {
        *(short *)(o + 0x162) = 1;
    } else {
        if (*(u8 *)(g + 0x49) == 0x17) {
            id = 0x22;
            for (p = D_8011F9D0; p < D_8011F9D0 + 0xC30; p += 0x68) {
                if (*(u16 *)p == id) {
                    e = p;
                    goto have;
                }
            }
            e = 0;
          have:
            if (e != 0) {
                func_80015978(e + 4, o + 0x15C);
                goto done;
            }
        }
        if (*(u16 *)o != 0x19) {
            func_80015978(o + 4, o + 0x15C);
        }
      done:
        *(short *)(o + 0x162) = 0;
    }
    if (*(short *)(o + 0x15A) == 0) {
        *(u16 *)(o + 0x154) = *(u16 *)(o + 6);
        *(u16 *)(o + 0x156) = *(u16 *)(o + 0xA);
        *(u16 *)(o + 0x158) = *(u16 *)(o + 0xE);
    }
    *(short *)(o + 0x15A) = 0;
}
