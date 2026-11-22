#ifndef BFM_MINE_MODEL_SEQUENCE_H
#define BFM_MINE_MODEL_SEQUENCE_H
#include "psx_types.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
typedef u32 MineSequenceWord;
typedef u32 MineSequenceModelRef;
typedef u32 MineSequenceScriptRef;
#else
#include <stdint.h>
typedef uintptr_t MineSequenceWord;
struct MineSequenceModel;
typedef struct MineSequenceModel *MineSequenceModelRef;
typedef const MineSequenceWord *MineSequenceScriptRef;
#endif

typedef struct MineSequenceModel {
    u8 pad00[0x20];
    MineSequenceWord pose;
} MineSequenceModel;

/* A native script is a typed sequence of integers and relocated pointers.
 * The original four-byte script slot is too narrow for a host pointer. */
typedef struct {
    u8 pad00[0x20];
    MineSequenceModelRef model;
    u8 pad24[0x44 - 0x20 - sizeof(MineSequenceModelRef)];
    u32 flags;
    u8 pad48[0xB4 - 0x48];
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    MineSequenceScriptRef script;
#else
    u32 script_identity;
#endif
    u16 timer;
    u16 saved;
    u8 pc, depth;
    u8 padBE[0xDC - 0xBE];
    u8 dc;
    u8 padDD[0x1A8 - 0xDD];
    u8 b1A8;
#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
    MineSequenceScriptRef script;
#endif
} MineSequenceActor;

extern u8 D_80126738[], D_80126838[], D_80126748[], D_80126848[];
extern s32 func_801549F8(MineSequenceActor *actor);
extern void func_80154B4C(MineSequenceActor *actor, MineSequenceWord value);
extern void func_80154A74(MineSequenceActor *actor, s32 index);
extern void func_801553C0(MineSequenceActor *actor);
extern void func_80155440(MineSequenceActor *actor);
extern void func_80155394(MineSequenceActor *actor);
extern void func_801553A8(MineSequenceActor *actor);
extern void func_80059A1C(MineSequenceWord source, u8 *destination);
void func_8015444C(MineSequenceActor *actor, s32 *count, u32 *flags, s32 *skip);

#endif
