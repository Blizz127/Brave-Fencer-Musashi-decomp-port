/* Owner re-derivation from retail assembly (clean input set). */
/* Four-phase frame-buffer/band-effect driver: phase 0 binds this frame's primitive pool and ordering table and clears the OT, phase 1 arms the step counter, phase 2 sets the draw environment and builds the bands, phase 3 advances them every second call and returns 1 once func_8001BADC reports completion. */
#include "psx_types.h"

typedef struct {
    unsigned addr : 24;
    unsigned len : 8;
} PrimTag;

typedef struct {
    u32 ot;     /* guest PrimTag *: &ordering table[0] */
    u8 pad4[8];
    u32 ot1;    /* guest PrimTag *: &ordering table[1] */
    u8 pad10[4];
} OtBuffer;

/* D_800AF630 viewed as a record; buf is the word the disassembly labels D_800B9A02. */
typedef struct {
    u8 pad0[0x160];
    u8 draw_env[0xA3D2 - 0x160]; /* +0x160: record handed to func_80059FC0; rest unknown */
    u16 buf;                     /* +0xA3D2: current frame-buffer slot */
} GameState;

extern u32 D_800747FC;
extern s32 D_80074800;
extern s32 D_80074804;
extern s32 D_8007480C;
extern u8 D_8007BA70[][0x13880];
extern u32 D_800A5E60;
extern OtBuffer D_800A651C[];
extern PrimTag D_800A6610[][4096];
extern GameState D_800AF630;

extern void func_80059BFC(u32, s32);
extern void func_80059FC0(u32);
extern void func_8001BA84(void);
extern void func_8001BB60(void);
extern s32 func_8001BADC(s32, s32);

s32 func_8001B86C(void)
{
    register GameState *state = &D_800AF630;

    switch (D_800747FC) {
    case 0:
        D_800A5E60 = (u32)D_8007BA70[state->buf];
        D_800A651C[state->buf].ot = (u32)D_800A6610[state->buf];
        D_800A651C[state->buf].ot1 = (u32)&D_800A6610[state->buf][1];
        func_80059BFC(D_800A651C[state->buf].ot, 2);
        func_8001BA84();
        D_80074800 = 0x80;
        D_800747FC++;
        break;
    case 1:
        D_80074804 = 1;
        D_800747FC = 2;
        break;
    case 2:
        func_80059FC0((u32)state->draw_env);
        func_80059BFC(D_800A651C[state->buf].ot, 2);
        func_8001BB60();
        D_800747FC++;
        /* fall through */
    case 3:
        if (D_80074804++ > 1) {
            D_80074804 = 1;
            if (func_8001BADC(D_8007480C, 1) != 0) {
                return 1;
            }
            D_8007480C++;
        } else {
            func_8001BADC(D_8007480C, 0);
        }
        break;
    }
    return 0;
}
