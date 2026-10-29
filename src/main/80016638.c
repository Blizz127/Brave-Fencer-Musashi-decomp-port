/* Owner re-derivation from retail assembly (clean input set). */
/* Allocates a 1-word primitive carrying a texture-page style code word ((mode << 5) & 0x9FF | 0xE1000000) and links it into ordering table slot idx. */
#include "psx_types.h"

typedef struct {
    u32 addr : 24;
    u32 pad : 8;
} OtEntry;

typedef struct {
    u32 addr : 24;
    u8 len;
    u32 code;
} CodePrim;

typedef struct {
    s32 unused;
    OtEntry *table;
} OtOwner;

extern void *func_80010A08(s32 size);

/* Returns retail's leftover v0: the 24-bit address ((u32)p & 0xFFFFFF) stored into the ordering-table entry. */
s32 func_80016638(OtOwner *ot, s32 idx, s32 mode)
{
    CodePrim *p;

    p = func_80010A08(8);
    p->len = 1;
    p->code = ((mode << 5) & 0x9FF) | 0xE1000000;
    p->addr = ot->table[idx].addr;
    return ot->table[idx].addr = (u32)p;
}
