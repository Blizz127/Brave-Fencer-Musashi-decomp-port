/* Owner re-derivation from retail assembly (clean input set). */
/* abi s32 (pointer) CONFIRMED: callers read v0 with lhu right after the jal to
 * pull fields out of the returned record. u16 param: masked once at entry
 * (andi 0xFFFF) and used only for the index. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 f2;
    u16 f4;
    u16 f6;
    u16 f8;
    u16 fA;
} Rec801563EC;

extern Rec801563EC D_80186CD4[];

Rec801563EC *func_801563EC(u16 idx)
{
    return &D_80186CD4[idx];
}
