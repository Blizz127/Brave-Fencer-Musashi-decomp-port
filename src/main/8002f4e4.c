/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up a request's table entry in the active map; returns the entry or 0 (0x7F = empty). */
#include "psx_types.h"

typedef struct {
    s16 unk0;
    s16 count;
    s16 base;
    s16 unk6;
    s16 *index;
    s16 rows;
    s16 stride;
    u16 *data;
} MapHdr;

typedef struct {
    s16 key;
    u8 pad[22];
} KeyEntry;

typedef struct {
    u8 unk0;
    u8 unk1;
    u8 slot;
    u8 row;
} Request;

extern s16 D_800A4EF0;
extern u32 D_800A4EE8;
extern KeyEntry D_800A4640[];

u16 func_8002F4E4(Request *req) {
    MapHdr *h;
    s32 k;
    s32 pitch;
    u16 *line;
    u16 v;

    if (D_800A4EF0 == 0) {
        return 0;
    }
    h = (MapHdr *)D_800A4EE8;
    if (h == 0) {
        return 0;
    }
    k = D_800A4640[req->slot].key;
    if (k < h->base) {
        return 0;
    }
    k -= h->base;
    if (k >= h->count) {
        return 0;
    }
    k = h->index[k];
    if (k == 0) {
        return 0;
    }
    if (req->row >= h->rows) {
        return 0;
    }
    k--;
    pitch = h->stride * 2;
    line = (u16 *)((u8 *)h->data + req->row * pitch);
    v = line[k];
    if (v != 0x7F) {
        return v;
    }
    return 0;
}
