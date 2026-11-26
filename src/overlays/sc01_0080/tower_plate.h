/* SPDX-License-Identifier: MIT */
#ifndef MUSASHI_SC01_0080_TOWER_PLATE_H
#define MUSASHI_SC01_0080_TOWER_PLATE_H

#include "psx_types.h"

typedef struct {
    u8 reserved00[0x12];
    s16 pose_y;
    u8 reserved14[0x18];
    u16 flags;
} PlateModel;

typedef union {
    u32 value;
    struct {
        u16 fraction;
        s16 whole;
    } parts;
} PlatePose;

typedef struct {
    s16 kind, state;
    u8 reserved04[2];
    s16 x;
    u8 reserved08[6];
    s16 z;
    u8 reserved10[0x0c];
    s32 timer;
    PlateModel *model;
    u8 reserved24[0x10];
    u16 credited;
    u8 reserved36[0x26];
    u16 flags;
    u8 reserved5e[0x7e];
    s16 home_x, home_z;
    u32 acceleration, velocity;
    PlatePose pose;
} TowerPlate;

extern char D_80183884[];
extern void func_8001C214(PlateModel *, void *);
extern void func_8012A828(TowerPlate *, void *);

#endif
