#ifndef BFM_FOREST_FIELD_UPDATE_H
#define BFM_FOREST_FIELD_UPDATE_H
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u8 pad000[4];
    /* 0x004 */ s32 pos[3];
    /* 0x010 */ u8 pad010[0x34];
    /* 0x044 */ u32 flags;
    /* 0x048 */ u8 pad048[0x50];
    /* 0x098 */ s16 spos[3];
    /* 0x09E */ u8 pad09E[0x112];
    /* 0x1B0 */ u32 timer;
} ForestFieldPlayer;

typedef struct {
    /* 0x00 */ u8 pad00[2];
    /* 0x02 */ s16 mode;
} ForestFieldModel;

typedef struct {
    /* 0x00 */ u8 b[8];
} ForestFieldPosition;

extern ForestFieldPlayer D_80126B58;
extern u32 D_80126B78; /* guest pointer: ForestFieldModel */
extern u32 D_80126B9C;
extern ForestFieldPosition D_80126BE0;
extern ForestFieldPosition D_801150F0;
extern s32 D_8011F730;
extern s16 D_801152B8;
extern s16 D_8012693A;
extern u8 D_80078EC1;
extern s32 D_80078EC8;

extern void func_80146FC4(ForestFieldPlayer *obj);
extern void func_80150A70(ForestFieldPlayer *obj);
extern void func_80147098(ForestFieldPlayer *obj);
extern void func_8014A638(ForestFieldPlayer *obj);
extern s32 func_80155458(ForestFieldPlayer *obj);
extern u8 func_80029104(void);
extern void func_8014ADE0(ForestFieldPlayer *obj);
extern void func_8014B350(ForestFieldPlayer *obj);
extern void func_8014B7A4(ForestFieldPlayer *obj);
extern void func_80161D58(ForestFieldPlayer *obj);
extern void func_80029344(void);
extern void func_80161A90(ForestFieldPlayer *obj);
extern void func_8014B504(ForestFieldPlayer *obj);
extern void func_80149BEC(ForestFieldPlayer *obj);
extern void func_8014B5D0(ForestFieldPlayer *obj);
extern void func_8014C99C(ForestFieldPlayer *obj);
extern void func_8014B190(ForestFieldPlayer *obj);
extern void func_80148648(ForestFieldPlayer *obj, u8 port);
extern void func_80149228(ForestFieldPlayer *obj);
extern s32 func_8014A59C(ForestFieldPlayer *obj);
extern void func_8016F14C(ForestFieldPlayer *obj);
extern void func_80154418(ForestFieldPlayer *obj);
extern void func_80154BE4(ForestFieldPlayer *obj);
extern void func_80165694(ForestFieldPlayer *obj);
extern void func_801654A8(ForestFieldPlayer *obj);
extern void func_8014A680(ForestFieldPlayer *obj);
extern void func_8014A6A8(ForestFieldPlayer *obj);
extern void func_8014A71C(ForestFieldPlayer *obj);
extern void func_80172588(ForestFieldPlayer *obj);
extern void func_801473DC(ForestFieldPlayer *obj);
extern u16 func_80015978(s32 *src, s16 *dst);


#if defined(__mips__) && !defined(__LP64__)
#define FIELD_MODEL(handle) ((ForestFieldModel *)(handle))
#else
extern ForestFieldModel *bfm_forest_field_model(u32 model_handle);
#define FIELD_MODEL(handle) bfm_forest_field_model(handle)
#endif
s32 func_80146128(void);
#endif
