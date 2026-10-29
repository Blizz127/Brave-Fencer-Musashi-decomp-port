/* Owner re-derivation from retail assembly (clean input set). */
/* Saves the live area tables back into area record `index` and marks the record as visited. */
#include "psx_types.h"

typedef struct {
    s32 words[0x98 / 4];
} AreaHeader;

typedef struct {
    u8 b[4];
} AreaQuad;

typedef struct {
    u8 pad0[0x24];
    AreaHeader header;
    u16 params[64];
    u8 flags[64];
    u8 map[256];
    AreaQuad entries[24];
} AreaRecord;

extern AreaHeader D_80078E78;
extern AreaRecord D_80078F28[];
extern s32 D_800A5E58[];
extern u16 D_800A6588[];
extern u8 D_800AE648[];
extern u8 D_800BA1B8[];
extern AreaQuad D_800BA2B8[];

void func_80029774(s32 index) {
    AreaRecord *rec;
    s32 i;

    rec = &D_80078F28[index];
    rec->header = D_80078E78;
    for (i = 0; i < 64; i++) {
        rec->params[i] = D_800A6588[i];
    }
    for (i = 0; i < 64; i++) {
        rec->flags[i] = D_800AE648[i];
    }
    for (i = 0; i < 256; i++) {
        rec->map[i] = D_800BA1B8[i];
    }
    for (i = 0; i < 24; i++) {
        rec->entries[i] = D_800BA2B8[i];
    }
    D_800A5E58[index] = 1;
}
