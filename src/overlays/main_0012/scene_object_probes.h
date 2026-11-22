#ifndef BFM_SCENE_OBJECT_PROBES_H
#define BFM_SCENE_OBJECT_PROBES_H

#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 pad;
} SceneProbeVector;

typedef struct Obj8014FE60 Obj8014FE60;

typedef struct Obj8014A2E4 {
    u16 active;
    u8 pad002[0x1E];
    u32 probe_object_address;
    u8 pad024[0x34];
    u32 probe_geometry_reference;
    u16 flags;
    u8 pad05E[0x4C];
    s16 hp;
    u8 pad0AC[0x60];
} Obj8014A2E4;

extern Obj8014A2E4 D_801202A0[];
extern SceneProbeVector D_801152A8;
s32 func_8014FE60(Obj8014FE60 *obj);
s32 func_80150170(u8 *obj);
int func_8014C278(char *source, char *target, int limit);

#endif
