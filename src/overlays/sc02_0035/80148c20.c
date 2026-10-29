/* Owner re-derivation from retail assembly (clean input set). */
/* Stores a halfword at +0x12 of the record pointed to by +0x20; returns 1. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    s16 v;
} Rec;

typedef struct {
    u8 pad0[0x20];
    Rec *rec;
} Obj;

int func_80148C20(Obj *o, s16 v) {
    o->rec->v = v;
    return 1;
}
