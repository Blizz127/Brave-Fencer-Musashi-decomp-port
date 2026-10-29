/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u8 pad0[0x217];
    u8 f217;      /* 0x217 */
};

extern Fn D_8011F738;
extern s32 D_8011F73C;
extern s32 D_8011F740;
extern void func_801718AC(s32 a0);

void func_80170CF0(Obj *o) {
    u8 v1;

    D_8011F738(o);
    v1 = o->f217;
    switch (v1) {
    case 0:
        return;
    case 1:
        func_801718AC(D_8011F73C);
        break;
    case 2:
        func_801718AC(D_8011F740);
        break;
    }
}
