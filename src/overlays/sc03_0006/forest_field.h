#ifndef BFM_FOREST_FIELD_H
#define BFM_FOREST_FIELD_H

#include "psx_types.h"

/* The scene scheduler supplies an original 32-bit actor slot to each phase. */
typedef void ForestFieldPhase(void *slot);
extern ForestFieldPhase *D_80190104[];

typedef struct {
    s16 x, y, z, pad;
} ForestVector;

typedef struct {
    s16 rotation[3][3], pad;
    s32 position[3];
} ForestMatrix;

typedef struct {
    s16 x, y, z, rx, ry, rz;
} ForestShardVector;

typedef struct {
    ForestShardVector shards[9];
} ForestMotion;

typedef union {
    struct { s32 x, y, z, pad; } direction;
    struct {
        u8 red, green, blue, reserved;
        u8 red_end, green_end, blue_end;
        u8 middle[5];
        u8 red3, green3, blue3, final;
    } tint;
} ForestLightEntry;

extern ForestLightEntry D_801F7380[], D_801F7390[], D_801F73A0[];
extern ForestMotion D_801E91DC;
extern s16 D_80115214, D_801152BA;

extern void *func_8001D074(s32 kind, s32 size);
extern void func_8001CD50(void *light, void *appearance);
extern void func_800233CC(void *appearance, s32 size);
extern u8 func_800291B4(s32 id);
extern void func_80049A1C(ForestVector *rotation, ForestMatrix *matrix);
extern s32 func_8005C49C(void);
extern void func_800D20C0(ForestVector *position, ForestVector *rotation, s32 mode);
extern void func_800D23D0(ForestVector *rotation);
extern s32 func_80128CFC(u16 resource);
extern void func_8012A828(void *actor, const void *animation);
extern void func_80178B18(void *actor, const void *script);
extern s32 func_80178970(void *actor);
extern void func_80178D18(void *actor);
extern void func_8017C294(void *actor, s16 mode);

#endif
